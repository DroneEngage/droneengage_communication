//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: WebSocket SSL client, asynchronous
//
//------------------------------------------------------------------------------


#include "../helpers/colors.hpp"
#include <boost/asio.hpp>
#include "andruav_comm_session.hpp"

#include <plog/Log.h> 
#include "plog/Initializers/RollingFileInitializer.h"


//------------------------------------------------------------------------------

static std::mutex g_i_mutex_writeText, g_i_mutex_on_read; 
static int wait_time = 30;

// Report a failure
void uavos::andruav_servers::CWSSession::fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Sends a WebSocket message and prints the response
void uavos::andruav_servers::CWSSession::run( char const* host, char const* port, char const* url_param)
{
        
        // Save these for later
        host_ = host;
        url_param_ = url_param;

        // Look up the domain name
        resolver_.async_resolve(
            host,
            port,
            beast::bind_front_handler(
                &CWSSession::on_resolve,
                shared_from_this()));
}

void uavos::andruav_servers::CWSSession::on_resolve(
        beast::error_code ec,
        tcp::resolver::results_type results)
{
    if(ec)
        return fail(ec, "resolve");

    // // Set a timeout on the operation
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(wait_time));

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(ws_).async_connect(
            results,
            beast::bind_front_handler(
                &CWSSession::on_connect,
                shared_from_this()));
}

void uavos::andruav_servers::CWSSession::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep)
{
    if(ec)
    {
        PLOG(plog::error) << "CWSSession::on_connect failed..code:" << ec; 
        m_connected = false;
        return fail(ec, "connect");
    }
    
    m_connected = true;

    // Update the host_ string. This will provide the value of the
    // Host HTTP header during the WebSocket handshake.
    // See https://tools.ietf.org/html/rfc7230#section-5.4
    host_ += ':' + std::to_string(ep.port());

    // Set a timeout on the operation
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(wait_time));

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if(! SSL_set_tlsext_host_name(
        ws_.next_layer().native_handle(),
        host_.c_str()))
    {
        ec = beast::error_code(static_cast<int>(::ERR_get_error()),
            net::error::get_ssl_category());
        PLOG(plog::error) << "CWSSession::on_connect failed..code:" << ec; 
        return fail(ec, "connect");
    }

    // Perform the SSL handshake
    ws_.next_layer().async_handshake(
        ssl::stream_base::client,
        beast::bind_front_handler(
            &CWSSession::on_ssl_handshake,
            shared_from_this()));
}

void uavos::andruav_servers::CWSSession::on_ssl_handshake(beast::error_code ec)
{
    if(ec)
        return fail(ec, "ssl_handshake");

    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    beast::get_lowest_layer(ws_).expires_never();

    // Set suggested timeout settings for the websocket
    ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::client));

    // Set a decorator to change the User-Agent of the handshake
    ws_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
        {
            req.set(http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) +
                " websocket-client-async-ssl");
        }));

    // Perform the websocket handshake
    ws_.async_handshake(host_, url_param_,
        beast::bind_front_handler(
        &CWSSession::on_handshake,
        shared_from_this()));
}

void uavos::andruav_servers::CWSSession::on_handshake(beast::error_code ec)
{
    if(ec)
        return fail(ec, "handshake");


    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(wait_time));
    // Read a message into our buffer
    ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &CWSSession::on_read,
                shared_from_this()));

       
}

void uavos::andruav_servers::CWSSession::on_write(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if(ec) {
        PLOG(plog::error) << "CWSSession::on_write failed..code:" << ec; 
        m_callback.onSocketError();
        return fail(ec, "write");
    }
}

void uavos::andruav_servers::CWSSession::on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
{
    const std::lock_guard<std::mutex> lock(g_i_mutex_on_read);

    #ifdef DEBUG_2
         std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "on_read: " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    boost::ignore_unused(bytes_transferred);

    if ((boost::asio::error::eof == ec) ||
        (boost::asio::error::connection_reset == ec) ||
        (boost::asio::error::connection_aborted == ec) ||
        (boost::asio::error::connection_refused == ec) ||
        (boost::asio::error::fault == ec))
    {
        // handle the disconnect.
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WebSocket Disconnected with Communication Server err:1:" << _NORMAL_CONSOLE_TEXT_ <<  std::endl;
        PLOG(plog::error) << "WebSocket Disconnected with Communication Server#1:";
        //m_connected = false;
        m_callback.onSocketError();
        return ;
    }
    else
    {
        // read the data 
        if(ec)
        {
            std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WebSocket Disconnected with Communication Server err:2:" << _NORMAL_CONSOLE_TEXT_ <<  std::endl;
            PLOG(plog::error) << "WebSocket Disconnected with Communication Server#2:";
            m_callback.onSocketError();
            return ;
        }
            

        std::ostringstream os;
        // copy buffer including NULLS
        os << beast::make_printable(buffer_.data());   
        std::string output = os.str();
        

        if (ws_.got_binary() == true)
        {
            m_callback.onBinaryMessageRecieved(output.c_str(), bytes_transferred);
           
        }
        else
        {
            m_callback.onTextMessageRecieved(output);
        }

        
        buffer_.clear();
       // delete result;
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &CWSSession::on_read,
                shared_from_this()));
        
    }
}


void uavos::andruav_servers::CWSSession::writeText (const std::string message)
{
    #ifdef DEBUG_GENERATE_FAILURE
        static int errr =0;
    #endif
    #ifdef DEBUG_2
         std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "writeText: " << message << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    try
    {
        const std::lock_guard<std::mutex> lock(g_i_mutex_writeText);
        ws_.binary(false);
        const int bytes = ws_.write(net::buffer(std::string(message)));
        if (bytes<=0)
        {
            PLOG(plog::error) << "WebSocket ws_.write error:" << bytes;
        }
        #ifdef DEBUG_GENERATE_FAILURE
        ++errr;
        #ifdef DEBUG
         std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "errr: " << errr << _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif
        if (errr%20==0)
        {
            throw std::invalid_argument("AddPositiveIntegers arguments must be positive");
        }
        #endif
            
    }
    catch (const std::exception& ex)
    {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WebSocket Disconnected with Communication Server on writeText" << _NORMAL_CONSOLE_TEXT_ <<   std::endl;
        PLOG(plog::error) << "WebSocket Disconnected with Communication Server on writeText."; 
        m_callback.onSocketError();
        return ;
    }
    
    return ;
}


void uavos::andruav_servers::CWSSession::writeBinary (const char * bmsg, const int& length)
{

    #ifdef DEBUG_2
         std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "write Binary" << std::endl;
    #endif
    try
    {
        const std::lock_guard<std::mutex> lock(g_i_mutex_writeText);
        ws_.binary(true);
        ws_.write(net::buffer(bmsg, length));
    }
    catch (const std::exception& ex)
    {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WebSocket Disconnected with Communication Server on writeBinary" << _NORMAL_CONSOLE_TEXT_ <<   std::endl;
        PLOG(plog::error) << "WebSocket Disconnected with Communication Server on writeBinary."; 
        m_callback.onSocketError();
        return ;
    }

    return ;
}


void uavos::andruav_servers::CWSSession::close ()
{
    if (!m_connected) return ;
    try
    {
        std::cout << _INFO_CONSOLE_TEXT << "Close websocket with Communication Server inprogress." << _NORMAL_CONSOLE_TEXT_ << std::endl;

        PLOG(plog::info) << "Close websocket with Communication Server inprogress."; 
        //ws_.close(websocket::close_code::normal);
        beast::error_code ec;
        ws_.next_layer().shutdown(ec);
        if (ec) {
            std::cout << "WS Error: " << ec.message() << std::endl;
        }
        m_connected = false;
        std::cout << _INFO_CONSOLE_TEXT << "Close websocket has been ShutDown." << _NORMAL_CONSOLE_TEXT_ << std::endl;

        
    }
    catch (const std::exception& ex)
    {
        m_connected = false;
    }
    catch (...)
    {
        m_connected = false;
    }
    
    std::cout << _INFO_CONSOLE_TEXT << "Close websocket with Communication Server done." << _NORMAL_CONSOLE_TEXT_ << std::endl;
    PLOG(plog::info) << "Close websocket with Communication Server done."; 
        
}
        
