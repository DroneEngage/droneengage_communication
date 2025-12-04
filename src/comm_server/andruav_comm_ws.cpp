#include <memory>
#include <thread>
#include <openssl/ssl.h> // Include the OpenSSL header for SSL/TLS functions
#include <openssl/err.h>
#include "../global.hpp"
#include "../helpers/colors.hpp"
#include "andruav_comm_ws.hpp"
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>

#include <plog/Log.h> 
#include "plog/Initializers/RollingFileInitializer.h"


using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;

void de::andruav_servers::CWSASession::run()
{
    try
    {
        // Resolve the host and port
        tcp::resolver::results_type endpoints = resolver_.resolve(host_, port_);
        if (endpoints.empty())
        {
            PLOG(plog::error) << "Failed to resolve endpoint: " << host_ << ":" << port_;
            m_connected.store(false);
            m_callback.onSocketError(); // Notify of initial connection failure
            return;
        }

        // Connect to the server
        auto ep = net::connect(get_lowest_layer(ws_), endpoints);
        m_connected.store(true);
        PLOG(plog::info) << "Connected to " << host_ << ":" << port_;

        // Perform the SSL handshake
        ws_.next_layer().handshake(ssl::stream_base::client);

        // Set SNI Hostname
        if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host_.c_str()))
        {
            boost::system::error_code ec;
            int ssl_error = SSL_get_error(ws_.next_layer().native_handle(), 0);
            PLOG(plog::error) << "SSL error setting SNI: " << ERR_error_string(ssl_error, nullptr);
            m_connected.store(false);
            m_callback.onSocketError();
            return;
        }

        // Set WebSocket headers (User-Agent and permessage-deflate)
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client");
                req.set(http::field::sec_websocket_extensions, "permessage-deflate");
            }));

        // Perform the WebSocket handshake
        beast::error_code ec;
        ws_.handshake(host_, url_param_, ec);
        if (ec)
        {
            PLOG(plog::error) << "WebSocket handshake failed: " << ec.message();
            m_connected.store(false);
            m_callback.onSocketError();
            return;
        }
        PLOG(plog::info) << "WebSocket handshake successful, compression: " << (ws_.got_text() ? "text" : "binary");

        // Start receiver thread
        m_thread_receiver = std::thread{[this]() {
            receive_message();
        }};
    }
    catch (const std::exception& e)
    {
        PLOG(plog::error) << "Error during connection setup: " << e.what();
        m_connected.store(false);
        m_callback.onSocketError();
        return;
    }
}

void de::andruav_servers::CWSASession::receive_message()
{
    beast::flat_buffer buffer;
    beast::error_code ec;

    while (m_connected.load()) {
        std::lock_guard<std::mutex> lock(g_i_mutex_on_read); // Lock the mutex
        
        // Check again after acquiring lock
        if (!m_connected.load()) break;
        
        try
        {
            ws_.read(buffer, ec);
            if (ec) {
                if (ec == beast::websocket::error::closed) {
                    PLOG(plog::info) << "WebSocket connection closed by the server.";
                    std::cout <<  "WebSocket connection closed by the server." << std::endl;
                } else if (ec == boost::asio::error::timed_out) {
                    PLOG(plog::warning) << "WebSocket read timeout.";
                    std::cout <<  "WebSocket read timeout." << std::endl;
                    // Consider if you want to try to recover or break
                } else if (ec == boost::asio::error::connection_reset) {
                    PLOG(plog::warning) << "Connection reset by peer.";
                    std::cout << "Connection reset by peer." << std::endl;
                } else if (ec == boost::asio::error::eof) {
                    PLOG(plog::info) << "End of file reached on WebSocket.";
                    std::cout << "WebSocket connection closed by the server." << std::endl;
                } else if (ec == boost::asio::error::operation_aborted) {
                    PLOG(plog::info) << "WebSocket read operation aborted (likely due to closing).";
                    std::cout << "WebSocket read operation aborted (likely due to closing)." << std::endl;
                } else {
                    PLOG(plog::error) << "WebSocket read error: " << ec.message();
                    std::cout <<  "WebSocket read error: " << ec.message() << std::endl;
                }
                m_connected.store(false);
                break; // Exit the loop on read error
            }

            std::ostringstream os;
            os << beast::make_printable(buffer.data());
            std::string output = os.str();

            #ifdef DEBUG
            #ifdef DEBUG_MSG
            std::cout << "Received message: " << buffer.size() << ":" << output << std::endl;
            #endif
            #endif

            if (ws_.got_binary())
            {
                m_callback.onBinaryMessageRecieved(output.c_str(), buffer.size());
            }
            else
            {
                m_callback.onTextMessageRecieved(output);
            }

            buffer.consume(buffer.size());
        }
        catch (const boost::system::system_error& e) {
            PLOG(plog::error) << "Boost system error during read: " << e.what();
            m_connected.store(false);
            break;
        }
        catch (const std::exception& ex) {
            PLOG(plog::error) << "Exception during read: " << ex.what();
            m_connected.store(false);
            break;
        }
        catch (...) {
            PLOG(plog::error) << "Unknown exception during read.";
            std::cout << "Unknown exception during read." << std::endl;
            m_connected.store(false);
            break;
        }
    }
    m_callback.onSocketClosed(); // Notify when the receive loop ends
}
    



void de::andruav_servers::CWSASession::close(beast::websocket::close_code code)
{
    // Use exchange to atomically set to false and check previous value
    // This ensures close() is only executed once even if called from multiple threads
    if (!m_connected.exchange(false)) return;

    beast::error_code ec;
    try
    {
        // Cancel any pending async operations on the underlying socket first
        // This will cause ws_.read() to return with operation_aborted error
        auto& lowest_layer = get_lowest_layer(ws_);
        if (lowest_layer.is_open())
        {
            lowest_layer.cancel(ec);
            if (ec) {
                PLOG(plog::warning) << "Error canceling socket operations: " << ec.message();
            }
        }

        // Now close the WebSocket gracefully if it's still open
        if (ws_.is_open())
        {
            ws_.close(code, ec);
            if (ec) {
                PLOG(plog::warning) << "Error closing WebSocket: " << ec.message();
            } else {
                PLOG(plog::info) << "WebSocket closed with code: " << static_cast<int>(code);
            }
        }
    }
    catch (const boost::exception& ex) {
        PLOG(plog::error) << "Caught BOOST_THROW_EXCEPTION during close.";
    }
    catch (const std::exception& ex) {
        PLOG(plog::error) << "Caught std::exception during close: " << ex.what();
    }
    catch (...) {
        PLOG(plog::error) << "Caught unknown exception during close.";
    }
}

void de::andruav_servers::CWSASession::close()
{
    close(websocket::close_code::normal);
}

void de::andruav_servers::CWSASession::writeText (const std::string& message)
{
    const std::lock_guard<std::mutex> lock(g_i_mutex_writeText);
    
    if (!m_connected.load()) return ;
    
    try
    {
        boost::system::error_code ec;
        ws_.binary(false);
        ws_.write(boost::asio::buffer(message), ec);  // Pass ec to capture errors
        if (ec)
        {
            std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WebSocket Disconnected with Communication Server on writeText: " << ec.message() << _NORMAL_CONSOLE_TEXT_ << std::endl;
            PLOG(plog::error) << "WebSocket Disconnected with Communication Server on writeText: " << ec.message();
            m_connected.store(false);
            m_callback.onSocketError();
            return;
        }
    } catch (const boost::exception& ex) {
        PLOG(plog::error) << "Caught BOOST_THROW_EXCEPTION on writeText";
        m_connected.store(false);
        m_callback.onSocketError();
        return ;
    } catch (const std::exception& ex) {
        PLOG(plog::error) << "Caught std::exception on writeText: " << ex.what();
        m_connected.store(false);
        m_callback.onSocketError();
        return ;
    } catch (...) {
        PLOG(plog::error) << "Caught unknown exception on writeText";
        m_connected.store(false);
        m_callback.onSocketError();
        return ;
    }
}

void de::andruav_servers::CWSASession::writeBinary (const char * bmsg, const int& length)
{
    const std::lock_guard<std::mutex> lock(g_i_mutex_writeText);
    
    if (!m_connected.load()) return ;
    
    try
    {
        boost::system::error_code ec;
        ws_.binary(true);
        ws_.write(boost::asio::buffer(bmsg, length), ec);
        if (ec)
        {
            std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WebSocket Disconnected with Communication Server on writeBinary: " << ec.message() << _NORMAL_CONSOLE_TEXT_ << std::endl;
            PLOG(plog::error) << "WebSocket Disconnected with Communication Server on writeBinary: " << ec.message();
            m_connected.store(false);
            m_callback.onSocketError();
            return;
        }
    } catch (const boost::exception& ex) {
        PLOG(plog::error) << "Caught BOOST_THROW_EXCEPTION on writeBinary";
        m_connected.store(false);
        m_callback.onSocketError();
        return ;
    } catch (const std::exception& ex) {
        PLOG(plog::error) << "Caught std::exception on writeBinary: " << ex.what();
        m_connected.store(false);
        m_callback.onSocketError();
        return ;
    } catch (...) {
        PLOG(plog::error) << "Caught unknown exception on writeBinary";
        m_connected.store(false);
        m_callback.onSocketError();
        return ;
    }
}

void de::andruav_servers::CWSASession::shutdown ()
{
    close(); // Initiate a graceful close
    try {
        if (m_thread_receiver.joinable()) {
            m_thread_receiver.join(); // Wait for the receiver thread to finish
            PLOG(plog::info) << "Receiver thread joined.";
        }
    }
    catch (const std::system_error& e) {
        PLOG(plog::error) << "Error joining thread during shutdown: " << e.what();
    }
}


std::unique_ptr<de::andruav_servers::CWSASession> de::andruav_servers::CWSAProxy::run1(char const* host, char const* port, char const* url_param, CCallBack_WSASession &callback)
{
    // Create a WebSocket client and connect to the server
    std::unique_ptr<de::andruav_servers::CWSASession> ptr = std::make_unique<de::andruav_servers::CWSASession>(io_context_1, std::string(host), std::string(port), std::string(url_param),callback);
    ptr.get()->run();
    return ptr;
}

std::unique_ptr<de::andruav_servers::CWSASession> de::andruav_servers::CWSAProxy::run2(char const* host, char const* port, char const* url_param, CCallBack_WSASession &callback)
{
    // Create a WebSocket client and connect to the server
    std::unique_ptr<de::andruav_servers::CWSASession> ptr = std::make_unique<de::andruav_servers::CWSASession>(io_context_2, std::string(host), std::string(port), std::string(url_param),callback);
    ptr.get()->run();
    return ptr;
}