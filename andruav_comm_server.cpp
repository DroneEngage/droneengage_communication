
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

#include "helpers/colors.hpp"
#include "andruav_comm_server.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>



void uavos::andruav_servers::CAndruavCommServer::connect (const std::string& server_ip, const std::string &server_port, const std::string& key, const std::string& party_id)
{
    try
    {
        
        auto const host = server_ip;
        auto const port = server_port;

        // The io_context is required for all I/O
        net::io_context ioc;

        // The SSL context is required, and holds certificates
        ssl::context ctx{ssl::context::tlsv12_client};

        // This holds the root certificate used for verification
        //load_root_certificates(ctx);

        // These objects perform our I/O
        tcp::resolver resolver{ioc};
        websocket::stream<beast::ssl_stream<tcp::socket>> ws{ioc, ctx};

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        net::connect(ws.next_layer().next_layer(), results.begin(), results.end());

        // Perform the SSL handshake
        ws.next_layer().handshake(ssl::stream_base::client);

        // Set a decorator to change the User-Agent of the handshake
        ws.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                // https://stackoverflow.com/questions/44331292/c-send-data-in-body-with-boost-asio-and-beast-library
                req.method(beast::http::verb::get);

                //req.set(beast::http::field::content_type, "application/x-www-form-urlencoded; charset=UTF-8");
                //req.prepare_payload();
                //Raw Data
                // req.set(beast::http::field::content_type, "text/plain");
                // req.body() = "Some raw data";

                // req.set(http::field::user_agent,
                //     "HEFNY TEST" +
                //         " websocket-client-coro"
                //         );
            }));

        // Perform the websocket handshake
        std::string url_param = "/?f=" + key + "&s=" + party_id;
        ws.handshake(server_ip, url_param);


        // Our message in this case should be stringified JSON-RPC request
        //ws.write(net::buffer(std::string(rpcJson)));

        // This buffer will hold the incoming message
        //beast::flat_buffer buffer;

        // Read a message into our buffer
        //ws.read(buffer);

        // Close the WebSocket connection
        ws.close(websocket::close_code::normal);

        // If we get here then the connection is closed gracefully

        // The make_printable() function helps print a ConstBufferSequence
        //std::cout << beast::make_printable(buffer.data()) << std::endl;
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        //return EXIT_FAILURE;
        return ;
    }
    //return EXIT_SUCCESS;
}


void uavos::andruav_servers::CAndruavCommServer::uninit()
{
    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: uninit " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}