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

#include "andruav_comm_session.hpp"
//------------------------------------------------------------------------------

static std::mutex g_i_mutex_writeText, g_i_mutex_on_read; 

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

    // Set a timeout on the operation
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

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
        return fail(ec, "connect");

    // Update the host_ string. This will provide the value of the
    // Host HTTP header during the WebSocket handshake.
    // See https://tools.ietf.org/html/rfc7230#section-5.4
    host_ += ':' + std::to_string(ep.port());

    // Set a timeout on the operation
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if(! SSL_set_tlsext_host_name(
        ws_.next_layer().native_handle(),
        host_.c_str()))
    {
        ec = beast::error_code(static_cast<int>(::ERR_get_error()),
            net::error::get_ssl_category());
        
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

    if(ec)
        return fail(ec, "write");
}

void uavos::andruav_servers::CWSSession::on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
{
    const std::lock_guard<std::mutex> lock(g_i_mutex_on_read);

    #ifdef DEBUG
         std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "on_read: " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    boost::ignore_unused(bytes_transferred);

    if ((boost::asio::error::eof == ec) ||
        (boost::asio::error::connection_reset == ec) ||
        (boost::asio::error::connection_aborted == ec) ||
        (boost::asio::error::connection_refused == ec) ||
        (boost::asio::error::fault == ec))
    {
        // handle the disconnect.
        std::cout << "WebSocket Disconnected with Andruav Server#1:" <<  std::endl;
        m_callback.onSocketError();
        return ;
    }
    else
    {
        // read the data 
        if(ec)
        {
            std::cout << "WebSocket Disconnected with Andruav Server#2:" <<  std::endl;
            m_callback.onSocketError();
            return ;
        }
            

        // The make_printable() function helps print a ConstBufferSequence
        
        std::cout << "This is a Data:" << beast::make_printable(buffer_.data()) << std::endl;

        char * result = new char[bytes_transferred+1];
        int i=0;
        for(auto const buffer : beast::buffers_range_ref(buffer_.data()))
        {
            char *c = (char *)(
                buffer.data());
            for (i=0; i<bytes_transferred;++i)
            {
                result[i] = c[i];
            }
            result[i]=0;
            break;
        }
        
        if (ws_.got_binary() == true)
        {
            m_callback.onBinaryMessageRecieved(result, bytes_transferred);
        }
        else
        {
            std::string output = std::string(result);
            m_callback.onTextMessageRecieved(output); //beast::buffers_to_string(buffer_.data()));    
        }

        delete result;
        
        buffer_.clear();
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &CWSSession::on_read,
                shared_from_this()));
        
    }
}


void uavos::andruav_servers::CWSSession::writeText (const std::string message)
{

    #ifdef DEBUG
         std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "writeText: " << message << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    const std::lock_guard<std::mutex> lock(g_i_mutex_writeText);
    
    ws_.write(net::buffer(std::string(message)));
}



void uavos::andruav_servers::CWSSession::close ()
{
    ws_.close(websocket::close_code::normal);
}
        
