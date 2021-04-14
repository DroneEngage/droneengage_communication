/*******************************************************************************************
 * 
 * U A V O S - C O M M U N I C A T I O N  Module
 * 
 *  
 * Author: Mohammad S. Hefny 
 * 
 * */

#include <iostream>
#include <signal.h>
#include <curl/curl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "./helpers/colors.hpp"
#include "./helpers/helpers.hpp"

#include "global.hpp"
#include "messages.hpp"
#include "configFile.hpp"
#include "udpCommunicator.hpp"

#include "andruav_auth.hpp"
#include "andruav_comm_server.hpp"
#include "uavos_modules_manager.hpp"



bool exit_me = false;


uavos::CConfigFile& cConfigFile = uavos::CConfigFile::getInstance();
uavos::comm::CUDPCommunicator& cUDPClient = uavos::comm::CUDPCommunicator::getInstance();  
uavos::CUavosModulesManager& cUavosModulesManager = uavos::CUavosModulesManager::getInstance();  


void onReceive (const char * jsonMessage, int len, struct sockaddr_in *  ssock);
void uninit ();





void onReceive (const char * jsonMessage, int len, struct sockaddr_in * ssock)
{
    static bool bFirstReceived = false;
        
    #ifdef DEBUG        
        std::cout << _INFO_CONSOLE_TEXT << "RX MSG: " << jsonMessage << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    
    Json jMsg;
    if (jsonMessage[len-1]==125 || (jsonMessage[len-2]==125))   // "}".charCodeAt(0)  IS TEXT / BINARY Msg  
    {
        jMsg = Json::parse(jsonMessage);
        //TODO INTERMODULE_COMMAND_TYPE field should be string.
        if ((!validateField(jMsg, INTERMODULE_COMMAND_TYPE, Json::value_t::string))
        || (!validateField(jMsg, ANDRUAV_PROTOCOL_MESSAGE_TYPE, Json::value_t::number_unsigned))
        )
        {
            // bad message format
            return ;
        }
        
        if (jMsg[INTERMODULE_COMMAND_TYPE].get<std::string>().compare(CMD_TYPE_INTERMODULE)==0)
        {
            bool forward = false;
            char *connected_ip = inet_ntoa(ssock->sin_addr);
            int port = ntohs(ssock->sin_port); 


            cUavosModulesManager.parseIntermoduleMessage(jMsg,
             ssock, forward);
            
        }
    }
    else
    {

    }
}


void connectToCommServer ()
{
    uavos::andruav_servers::CAndruavAuthenticator& andruav_auth = uavos::andruav_servers::CAndruavAuthenticator::getInstance();
    uavos::andruav_servers::CAndruavCommServer& andruav_server = uavos::andruav_servers::CAndruavCommServer::getInstance();
    andruav_server.connect(andruav_auth.m_comm_server_ip, std::to_string(andruav_auth.m_comm_server_port), andruav_auth.m_comm_server_key, "oppa");
}

bool autehticateToServer ()
{
    uavos::andruav_servers::CAndruavAuthenticator& andruav_auth = uavos::andruav_servers::CAndruavAuthenticator::getInstance();

    const Json& jsonConfig = cConfigFile.GetConfigJSON();
    
    if ((!validateField(jsonConfig,"auth_ip", Json::value_t::string))
     || (validateField(jsonConfig,"auth_port", Json::value_t::number_unsigned) == false)
     )
    {
        std::cout << std::to_string(validateField(jsonConfig,"auth_ip", Json::value_t::string)) << std::endl;
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "FATAL:: Missing login info in config file !!" <<_NORMAL_CONSOLE_TEXT_ << std::endl;
        exit(1);
    }

    //TODO: Move urls to auth class.
    std::string url =  "https://" + jsonConfig["auth_ip"].get<std::string>() + ":" + std::to_string(jsonConfig["auth_port"].get<int>()) +  "/agent/al/";
    //std::string url =  "https://andruav.com:19408/w/wl/";
    std::string param =  "acc=" + jsonConfig["userName"].get<std::string>()
                +  "&pwd=" + jsonConfig["accessCode"].get<std::string>() 
                + "&gr=1&app=uavos&ver=" + jsonConfig["version"].get<std::string>() 
                + "&ex=uavos&at=d";

    std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "Auth Server " << _LOG_CONSOLE_TEXT << " connection established " << _SUCCESS_CONSOLE_BOLD_TEXT_ << " successfully" << _NORMAL_CONSOLE_TEXT_ << std::endl;
#ifdef DEBUG
    std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "Auth URL: " << _TEXT_BOLD_HIGHTLITED_ << url << "?" << param << _NORMAL_CONSOLE_TEXT_ << std::endl;
#endif
       
    const int res = andruav_auth.getAuth (url, param);


    if ((res !=CURLE_OK) || (andruav_auth.getErrorCode() !=0))
    {
        // error 
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Andruav Authentication Failed !!" <<_NORMAL_CONSOLE_TEXT_ << std::endl;
        return false;
    }
    else
    {
        std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "Andruav Authentication Succeeded !!" <<_NORMAL_CONSOLE_TEXT_ << std::endl;
        return true;
    }
}

void initSockets()
{

    const Json& jsonConfig = cConfigFile.GetConfigJSON();
    
    // UDP Server
    cUDPClient.init(jsonConfig["s2s_udp_target_ip"].get<std::string>().c_str(),
            std::stoi(jsonConfig["s2s_udp_target_port"].get<std::string>().c_str()),
            jsonConfig["s2s_udp_listening_ip"].get<std::string>().c_str() ,
            std::stoi(jsonConfig["s2s_udp_listening_port"].get<std::string>().c_str()));
    
    
    cUDPClient.SetMessageOnReceive (&onReceive);
    cUDPClient.start();
}

/**
 * initialize components
 **/
void init (int argc, char *argv[]) 
{

    std::string configName = "config.module.json";
    if (argc > 1)
    {
        configName = argv[1];
    }


    // Reading Configuration
    std::cout << std::endl << _SUCCESS_CONSOLE_BOLD_TEXT_ << "=================== " << "STARTING PLUGIN ===================" << _NORMAL_CONSOLE_TEXT_ << std::endl;

    
    cConfigFile.InitConfigFile (configName.c_str());
    

    
    bool res = autehticateToServer();

    if (res == true)
    {
        connectToCommServer ();
    }
    
    initSockets();


    // std::cout << "RESPONSE:" << response << std::endl;
    // Json json_response = Json::parse(response);
    // std::cout << "RESPONSE: JSON " << json_response.dump() << std::endl;
    // std::cout << "RESPONSE: JSON " << json_response["sid"].get<std::string>() << std::endl;
    
}

void uninit ()
{

    uavos::andruav_servers::CAndruavCommServer& andruav_server = uavos::andruav_servers::CAndruavCommServer::getInstance();
    
    andruav_server.uninit();
    
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: Unint" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    
    
    cUDPClient.stop();

    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: Unint_after Stop" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    
    // end program here
	exit(0);
}


// ------------------------------------------------------------------------------
//   Quit Signal Handler
// ------------------------------------------------------------------------------
// this function is called when you press Ctrl-C
void quit_handler( int sig )
{
	std::cout << _INFO_CONSOLE_TEXT << std::endl << "TERMINATING AT USER REQUEST" <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
	
	try 
    {
        exit_me = true;
        uninit();
	}
	catch (int error){}

    exit(0);
}




int main (int argc, char *argv[])
{
    signal(SIGINT,quit_handler);
	
    init (argc, argv);

    while (!exit_me)
    {
       std::this_thread::sleep_for(std::chrono::seconds(1));
       
    }

}