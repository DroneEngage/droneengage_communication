#ifndef ANDRUAV_COMM_SESSION_H_
#define ANDRUAV_COMM_SESSION_H_

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


#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

namespace uavos
{
namespace andruav_servers
{


class CCallBack_WSSession
{
    public:

    virtual void onBinaryMessageRecieved(const char * message, const std::size_t datalength)   {};                                                          
    virtual void onTextMessageRecieved(const std::string& JsonMessage)   {};                                                          
    virtual void onSocketClosed()                                   {}; 
    virtual void onSocketError ()                                   {};
    
};


// Sends a WebSocket message and prints the response
class CWSSession : public std::enable_shared_from_this<CWSSession>
{
        

    public:
        // Resolver and socket require an io_context
        explicit
        CWSSession(net::io_context& ioc, ssl::context& ctx, CCallBack_WSSession &callback)
            : resolver_(net::make_strand(ioc))
            , ws_(net::make_strand(ioc), ctx)
            , m_callback(callback)
        {
        }

        // Start the asynchronous operation
    public:

        void run(char const* host, char const* port, char const* url_param);

        void writeText (const std::string message);
        void writeBinary (const char * bmsg, const int& length);
        
        /**
         * @brief Close socket normally.
         * 
         */
        void close ();

        void on_resolve(beast::error_code ec, tcp::resolver::results_type results);

        void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep); 
            
        void on_ssl_handshake(beast::error_code ec);
        
        void on_handshake(beast::error_code ec);

        void on_write(beast::error_code ec, std::size_t bytes_transferred);

        void on_read(beast::error_code ec, std::size_t bytes_transferred);

        
    private:

        void fail(beast::error_code ec, char const* what);

    private:

        tcp::resolver resolver_;
        websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_;
        beast::flat_buffer buffer_;
        std::string host_;
        std::string url_param_;

        uavos::andruav_servers::CCallBack_WSSession &m_callback;
};

};
};

#endif