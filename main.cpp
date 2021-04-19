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

#include "andruav_unit.hpp"
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
        // Test Message if Binary or Text
        jMsg = Json::parse(jsonMessage);
        //TODO INTERMODULE_COMMAND_TYPE field should be string.
        if ((!validateField(jMsg, INTERMODULE_COMMAND_TYPE, Json::value_t::string))
        || (!validateField(jMsg, ANDRUAV_PROTOCOL_MESSAGE_TYPE, Json::value_t::number_unsigned))
        )
        {
            // bad message format
            return ;
        }
        
        // if (jMsg[INTERMODULE_COMMAND_TYPE].get<std::string>().compare(CMD_TYPE_INTERMODULE)==0)
        // {
            bool forward = false;
            char *connected_ip = inet_ntoa(ssock->sin_addr);
            int port = ntohs(ssock->sin_port); 


            cUavosModulesManager.parseIntermoduleMessage(jMsg,
             ssock, forward);
            
        // }
    }
    else
    {
        // Binary Message
            std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _ERROR_CONSOLE_BOLD_TEXT_ << "Binary Message from UDP ... NOT IMPLEMENTED" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    
    }
}


void defineMe()
{
    const Json& jsonConfig = cConfigFile.GetConfigJSON();
    
    uavos::CAndruavUnitMe& m_andruavMe = uavos::CAndruavUnitMe::getInstance();
    uavos::ANDRUAV_UNIT_INFO&  unit_info = m_andruavMe.getUnitInfo();
    
    unit_info.party_id = jsonConfig["partyID"].get<std::string>();
    unit_info.unit_name = jsonConfig["userName"].get<std::string>();
    unit_info.description = jsonConfig["unitDescription"].get<std::string>();
}


/**
 * @brief Establish connection with Communication Server
 * 
 * @Warning THIS FUNCTION DOES NOT RETURN....
 * TODO: Call it in a thread.
 */
void connectToCommServer ()
{
    uavos::andruav_servers::CAndruavCommServer& andruav_server = uavos::andruav_servers::CAndruavCommServer::getInstance();
    andruav_server.start();
}



/**
 * @brief Initialize UDP connection with other modules.
 * 
 */
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
    
    defineMe();
    
    initSockets();

    connectToCommServer ();

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