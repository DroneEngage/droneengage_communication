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
#include <pthread.h>

#include "./helpers/colors.hpp"
#include "./helpers/helpers.hpp"
#include "./helpers/util_rpi.hpp"

#include "global.hpp"
#include "messages.hpp"
#include "configFile.hpp"
#include "udpCommunicator.hpp"

#include "./comm_server/andruav_unit.hpp"
#include "./comm_server/andruav_comm_server.hpp"
#include "./uavos/uavos_modules_manager.hpp"



bool exit_me = false;


uavos::CConfigFile& cConfigFile = uavos::CConfigFile::getInstance();
uavos::comm::CUDPCommunicator& cUDPClient = uavos::comm::CUDPCommunicator::getInstance();  
uavos::CUavosModulesManager& cUavosModulesManager = uavos::CUavosModulesManager::getInstance();  

pthread_t m_scheduler;
bool exit_scheduler = false;


// void onReceive (const char * jsonMessage, int len, struct sockaddr_in *  ssock);
// void uninit ();




void * scheduler (void *args)
{

    uavos::andruav_servers::CAndruavCommServer& andruav_server = uavos::andruav_servers::CAndruavCommServer::getInstance();
    
    uint64_t hz_1 = 0;
    uint64_t every_5_sec = 0;

    while (!exit_scheduler)
    {
        hz_1++;
        every_5_sec ++;

        if (hz_1 % 10 == 0)
        {
            // if (andruav_server.getStatus() == SOCKET_STATUS_REGISTERED)
            // {
            //     andruav_server.API_sendID("");
            // }
        }

        if (every_5_sec % 50 == 0)
        {
            if (andruav_server.getStatus() == SOCKET_STATUS_REGISTERED)
            {
                cUavosModulesManager.HandleDeadModules();
                andruav_server.API_sendID("");
            }
        }


        usleep(100000); // 10Hz
    }

    return NULL;
}


void onReceive (const char * message, int len, struct sockaddr_in * ssock)
{
        
    #ifdef DEBUG        
        std::cout << _INFO_CONSOLE_TEXT << "RX MSG: " << message << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    cUavosModulesManager.parseIntermoduleMessage(message, len, ssock);

        // char *connected_ip = inet_ntoa(ssock->sin_addr);
        // int port = ntohs(ssock->sin_port); 
}


void initScheduler()
{
    const int result = pthread_create( &m_scheduler, NULL, &scheduler, NULL );
    if ( result ) throw result;

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

    std::string serial;
    if (helpers::CUtil_Rpi::getInstance().get_cpu_serial(serial)!=-1)
    {
        std::cout << "Unique Key :" << serial << std::endl;
    }

    std::string configName = "config.module.json";
    if (argc > 1)
    {
        configName = argv[1];
    }


    // Reading Configuration
    std::cout << std::endl << _SUCCESS_CONSOLE_BOLD_TEXT_ << "=================== " << "STARTING UAVOS COMMUNICATOR ===================" << _NORMAL_CONSOLE_TEXT_ << std::endl;

    
    cConfigFile.InitConfigFile (configName.c_str());
    
    defineMe();
    

    initScheduler();

    initSockets();

    connectToCommServer ();

}

void uninit ()
{

    uavos::andruav_servers::CAndruavCommServer& andruav_server = uavos::andruav_servers::CAndruavCommServer::getInstance();

    exit_me = true;
    exit_scheduler = true;
    // wait for exit
	pthread_join(m_scheduler ,NULL);
	
    andruav_server.uninit();
    
    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: Unint" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    
    cUDPClient.stop();

    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: Unint_after Stop" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
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
        uninit();
	}
	catch (int error)
    {
        #ifdef DEBUG
            std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: quit_handler" << std::to_string(error)<< _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif

    }

    exit(0);
}




int main (int argc, char *argv[])
{
    signal(SIGINT,quit_handler);
    signal(SIGTERM,quit_handler);
	
    init (argc, argv);

    while (!exit_me)
    {
       std::this_thread::sleep_for(std::chrono::seconds(1));
       
    }

}