

#include <cstdlib>
#include <string>

#define BOOST_BEAST_ALLOW_DEPRECATED


#include "helpers/colors.hpp"
#include "andruav_comm_server.hpp"




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
        std::string url_param = "/?f=" + key + "&s=" + party_id;
        
        // Launch the asynchronous operation
        //_session = std::make_shared<uavos::andruav_servers::session>(ioc, ctx)->run(host.c_str(), port.c_str(), url_param.c_str());
        std::make_shared<uavos::andruav_servers::CWSSession>(ioc, ctx)->run(host.c_str(), port.c_str(), url_param.c_str());

        // Run the I/O service. The call will return when
        // the socket is closed.
        ioc.run();


        
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