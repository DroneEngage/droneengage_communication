

#include <cstdlib>
#include <string>

#define BOOST_BEAST_ALLOW_DEPRECATED



#include "./helpers/colors.hpp"
#include "./helpers/helpers.hpp"

#include "messages.hpp"
#include "andruav_comm_server.hpp"




void uavos::andruav_servers::CAndruavCommServer::connect (const std::string& server_ip, const std::string &server_port, const std::string& key, const std::string& party_id)
{
    try
    {
        
        m_host = std::string(server_ip);
        m_port = std::string(server_port);
        // The io_context is required for all I/O
        net::io_context ioc;

        // The SSL context is required, and holds certificates
        ssl::context ctx{ssl::context::tlsv12_client};

        // This holds the root certificate used for verification
        //load_root_certificates(ctx);
        m_url_param = "/?f=" + key + "&s=" + party_id;
        
        // Launch the asynchronous operation
        _cwssession = std::shared_ptr<uavos::andruav_servers::CWSSession>(new uavos::andruav_servers::CWSSession(ioc, ctx, *this));
        _cwssession.get()->run(m_host.c_str(), m_port.c_str(), m_url_param.c_str());
        //->run(host.c_str(), port.c_str(), url_param.c_str());
        //uavos::andruav_servers::CWSSession * ptr = new uavos::andruav_servers::CWSSession(ioc, ctx);
        //_cwssession = std::unique_ptr<uavos::andruav_servers::CWSSession>(ptr);
        //ptr->run(host.c_str(), port.c_str(), url_param.c_str());

        // Run the I/O service. The call will return when
        // the socket is closed.
        ioc.run();
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return ;
    }
}


void uavos::andruav_servers::CAndruavCommServer::onSocketError()
{
    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: onSocketError " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << " Andruav Server Connected: Error "  << _NORMAL_CONSOLE_TEXT_ << std::endl;
    
    m_status = SOCKET_STATUS_ERROR;
}



void uavos::andruav_servers::CAndruavCommServer::onTextMessageRecieved(const std::string jsonMessage)
{
    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: onMessageRecieved " << jsonMessage << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    Json jMsg;
    jMsg = Json::parse(jsonMessage);
    if (!validateField(jMsg, INTERMODULE_COMMAND_TYPE, Json::value_t::string))
    {
        // bad message format
        return ;
    }

    if (!validateField(jMsg, ANDRUAV_PROTOCOL_MESSAGE_TYPE, Json::value_t::number_unsigned))
    {
        // bad message format
        return ;
    }

    if (jMsg[INTERMODULE_COMMAND_TYPE].get<std::string>().compare(CMD_TYPE_SYSTEM_MSG)==0)
    {
        const int command_type = jMsg[ANDRUAV_PROTOCOL_MESSAGE_TYPE].get<int>();
        switch (command_type)
        {
        case TYPE_AndruavSystem_ConnectedCommServer:
        {
            // example onMessageRecieved {"ty":"s","mt":9007,"ms":{"s":"OK:connected:tcp:192.168.1.144:37196"}}
            Json message_cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
            if (message_cmd["s"].get<std::string>().find("OK")==0)
            {
                std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << " Andruav Server Connected: Success "  << _NORMAL_CONSOLE_TEXT_ << std::endl;
                m_status = SOCKET_STATUS_REGISTERED;
                _cwssession.get()->writeText("OK");
            }
            else
            {
                std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << " Andruav Server Connected: Failed "  << _NORMAL_CONSOLE_TEXT_ << std::endl;
                m_status = SOCKET_STATUS_ERROR;
            }
        }
            break;
        
        default:
            break;
        }
    }
}


void uavos::andruav_servers::CAndruavCommServer::uninit()
{
    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: uninit " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}