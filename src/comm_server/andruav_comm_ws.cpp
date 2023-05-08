#include <memory>
#include <openssl/ssl.h> // Include the OpenSSL header for SSL/TLS functions
#include <openssl/err.h>
#include "../global.hpp"
#include "../helpers/colors.hpp"
#include "andruav_comm_ws.hpp"


#include <plog/Log.h> 
#include "plog/Initializers/RollingFileInitializer.h"

static std::mutex g_i_mutex_writeText, g_i_mutex_on_read; 

void uavos::andruav_servers::CWSASession::run()
{
    try
    {
        // Resolve the host and port
        tcp::resolver::results_type endpoints = resolver_.resolve(host_, port_);

        // Make sure there is at least one endpoint
        if (endpoints.empty())
        {
            throw std::runtime_error("Failed to resolve endpoint");
        }

        // Connect to the server
        auto ep = net::connect(get_lowest_layer(ws_), endpoints);
        m_connected = true;

        // Perform the SSL handshake
        ws_.next_layer().handshake(ssl::stream_base::client);

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host_.c_str()))
        {
            throw std::runtime_error("Failed to set SNI hostname");
        }

        // Set a decorator to change the User-Agent of the handshake
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client");
            }));

        // Perform the WebSocket handshake
        beast::error_code ec;
        ws_.handshake(host_ , url_param_, ec);
        if (ec)
        {
            throw std::runtime_error("WebSocket handshake failed: " + ec.message());
        }

        m_thread_receiver = std::thread {[&](){ receive_message(); }};

        //io_context_.run();
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return ;
    }
}

void uavos::andruav_servers::CWSASession::receive_message()
{
    // This buffer will hold the incoming message
    beast::flat_buffer buffer;
    beast::error_code ec;
    
    while (m_connected) {
        // boost::asio::steady_timer timer(io_context_);
        // timer.expires_after(std::chrono::seconds(5));
        // timer.async_wait([](const boost::system::error_code& ec)
        // {
        //     if (ec == boost::asio::error::operation_aborted)
        //     {
        //         // The timer was cancelled, so the receive operation completed successfully
        //         std::cout << "OKOK" << std::endl;
        //         return;
        //     }
        //     else
        //     {
        //         // The timer expired, so the receive operation timed out
        //         std::cout << "OFFFF" << std::endl;
        //         throw boost::system::system_error(boost::asio::error::timed_out);
        //     }
        // });

        // Read a message into our buffer
        {
        const std::lock_guard<std::mutex> lock(g_i_mutex_writeText);
        
        ws_.read(buffer, ec);
        if (!m_connected) return ;
        if (ec) {
            if (ec == beast::websocket::error::closed) {
                // WebSocket connection closed by the server
                std::cout << "WebSocket connection closed by the server" << std::endl;
            } else if (ec == boost::asio::error::timed_out) {
                // Timeout occurred
                std::cout << "WebSocket read timeout" << std::endl;
            } else if (ec == boost::asio::error::connection_reset) {
                // Connection reset by peer
                std::cout << "Connection reset by peer" << std::endl;
            } else if (ec == boost::asio::error::eof) {
                // End of file reached
                std::cout << "End of file reached" << std::endl;
            } 
            else {
                // boost::asio::error::operation_aborted
                // Other WebSocket or networking error
                std::cout << "WebSocket read error: " << ec.message() << std::endl;
            }
            break;
        }
        }
        

        // Print the received message
        std::ostringstream os;
        // copy buffer including NULLS
        os << beast::make_printable(buffer.data());   
        std::string output = os.str();

        std::cout << "Received message: " << output << std::endl;
        

        if (ws_.got_binary() == true)
        {
            m_callback.onBinaryMessageRecieved(output.c_str(), buffer.size());
           
        }
        else
        {
            m_callback.onTextMessageRecieved(output);
        }

        buffer.consume(buffer.size());

    }
    
    // Close the WebSocket connection
    close(websocket::close_code::normal);
    m_callback.onSocketError();
}
    



void uavos::andruav_servers::CWSASession::close(beast::websocket::close_code code)
{
    if (!m_connected) return ;

    m_connected = false;
    
    beast::error_code ec;
    ws_.close(websocket::close_code::normal, ec);
    if (ec) {
        // Handle the error
        std::cerr << "Error closing WebSocket: " << ec.message() << std::endl;
        return ;
    }

    while (ws_.is_open()) {
        usleep(1000);
    }
}

void uavos::andruav_servers::CWSASession::close()
{
    close(websocket::close_code::normal);
}

void uavos::andruav_servers::CWSASession::writeText (const std::string& message)
{
    
    const std::lock_guard<std::mutex> lock(g_i_mutex_writeText);
    
    if (!m_connected) return ;
    
    try
    {
        boost::system::error_code ec;
        ws_.binary(false);
        ws_.write(boost::asio::buffer(message), ec);
        if (ec)
        {
            std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WebSocket Disconnected with Communication Server on writeBinary: " << ec.message() << _NORMAL_CONSOLE_TEXT_ << std::endl;
            PLOG(plog::error) << "WebSocket Disconnected with Communication Server on writeBinary: " << ec.message();
            close(websocket::close_code::going_away);
            m_callback.onSocketError();
            return;
        }
    } catch (const boost::exception& ex) {
    // Handle the exception
    std::cerr << "Caught BOOST_THROW_EXCEPTION: "  << std::endl;
    } catch (const std::exception& ex) {
        // Handle other exceptions derived from std::exception
        std::cerr << "Caught std::exception: " << ex.what() << std::endl;
    } catch (...) {
        // Handle any other uncaught exceptions
        std::cerr << "Caught unknown exception" << std::endl;
    }
}

void uavos::andruav_servers::CWSASession::writeBinary (const char * bmsg, const int& length)
{
    
    const std::lock_guard<std::mutex> lock(g_i_mutex_writeText);
    
    if (!m_connected) return ;
    
    try
    {
        boost::system::error_code ec;
        ws_.binary(true);
        ws_.write(boost::asio::buffer(bmsg, length), ec);
        if (ec)
        {
            std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WebSocket Disconnected with Communication Server on writeBinary: " << ec.message() << _NORMAL_CONSOLE_TEXT_ << std::endl;
            PLOG(plog::error) << "WebSocket Disconnected with Communication Server on writeBinary: " << ec.message();
            close(websocket::close_code::going_away);
            m_callback.onSocketError();
            return;
        }
    }
    catch (const std::exception& ex)
    {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WebSocket Disconnected with Communication Server on writeBinary" << _NORMAL_CONSOLE_TEXT_ <<   std::endl;
        PLOG(plog::error) << "WebSocket Disconnected with Communication Server on writeBinary."; 
        m_callback.onSocketError();
        return ;
    }

}

void uavos::andruav_servers::CWSASession::shutdown ()
{
    close();
    
}


std::unique_ptr<uavos::andruav_servers::CWSASession> uavos::andruav_servers::CWSAProxy::run(char const* host, char const* port, char const* url_param, CCallBack_WSASession &callback)
{
    // The io_context is required for all I/O
    boost::asio::io_context io_context;

    // Create a WebSocket client and connect to the server
    std::unique_ptr<uavos::andruav_servers::CWSASession> ptr = std::make_unique<uavos::andruav_servers::CWSASession>(io_context, std::string(host), std::string(port), std::string(url_param),callback);
    ptr.get()->run();
    return std::move(ptr);
}