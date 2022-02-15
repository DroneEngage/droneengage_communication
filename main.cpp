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
//#include <pthread.h>
#include <thread>

#include <getopt.h>



#include "version.h"
#include "./helpers/colors.hpp"
#include "./helpers/helpers.hpp"
#include "./helpers/util_rpi.hpp"
#include "./helpers/getopt_cpp.hpp"

#include "global.hpp"
#include "messages.hpp"
#include "configFile.hpp"
#include "udpCommunicator.hpp"

#include "./comm_server/andruav_unit.hpp"
#include "./comm_server/andruav_comm_server.hpp"
#include "./comm_server/andruav_facade.hpp"
#include "./uavos/uavos_modules_manager.hpp"
//#include "./hal_linux/rpi_gpio.hpp"
#include "./notification_module/leds.hpp"

using namespace boost;
using namespace std;


bool exit_me = false;


uavos::CConfigFile& cConfigFile = uavos::CConfigFile::getInstance();
uavos::comm::CUDPCommunicator& cUDPClient = uavos::comm::CUDPCommunicator::getInstance();  
uavos::CUavosModulesManager& cUavosModulesManager = uavos::CUavosModulesManager::getInstance();  

//hal_linux::CRPI_GPIO &cGPIO = hal_linux::CRPI_GPIO::getInstance();
notification::CLEDs &cLeds = notification::CLEDs::getInstance();

std::thread m_scheduler;
bool exit_scheduler = false;

    
static std::string configName = "config.module.json";


void quit_handler( int sig );
/**
 * @brief display version info
 * 
 */
void _version (void)
{
    std::cout << std::endl << _SUCCESS_CONSOLE_BOLD_TEXT_ "Drone-Engage Communicator Server version " << _INFO_CONSOLE_TEXT << version_string << _NORMAL_CONSOLE_TEXT_ << std::endl;
}


/**
 * @brief display help for -h command argument.
 * 
 */
void _usage(void)
{
    _version ();
    std::cout << std::endl << _INFO_CONSOLE_TEXT "Options" << _NORMAL_CONSOLE_TEXT_ << std::ends;
    std::cout << std::endl << _INFO_CONSOLE_TEXT "\t--config:          -c ./config.json   default [./config.module.json]" << _NORMAL_CONSOLE_TEXT_ << std::ends;
    std::cout << std::endl << _INFO_CONSOLE_TEXT "\t--version:         -v" << _NORMAL_CONSOLE_TEXT_ << std::endl;
}


void scheduler ()
{

    uavos::andruav_servers::CAndruavCommServer& andruav_server = uavos::andruav_servers::CAndruavCommServer::getInstance();
    uavos::andruav_servers::CAndruavFacade& andruav_facade = uavos::andruav_servers::CAndruavFacade::getInstance();
    
    uint64_t hz_1 = 0;
    uint64_t every_5_sec = 0;

    while (!exit_scheduler)
    {
        hz_1++;
        every_5_sec ++;
        
        cLeds.update();

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
                cUavosModulesManager.handleDeadModules();
                andruav_facade.API_sendID("");
            }
        }

        usleep(100000); // 10Hz
    }

    return ;
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
    m_scheduler = std::thread (scheduler);
    //const int result = pthread_create( &m_scheduler, NULL, &scheduler, NULL );
    //if ( result ) throw result;
}


void defineMe()
{
    const Json& jsonConfig = cConfigFile.GetConfigJSON();
    uavos::CAndruavUnitMe& m_andruavMe = uavos::CAndruavUnitMe::getInstance();
    uavos::ANDRUAV_UNIT_INFO&  unit_info = m_andruavMe.getUnitInfo();
    
    unit_info.party_id = jsonConfig["partyID"].get<std::string>();
    unit_info.unit_name = jsonConfig["userName"].get<std::string>();
    unit_info.unit_name = jsonConfig["userName"].get<std::string>();
    unit_info.group_name = jsonConfig["groupID"].get<std::string>();
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
    cUDPClient.init(jsonConfig["s2s_udp_listening_ip"].get<std::string>().c_str() ,
                    std::stoi(jsonConfig["s2s_udp_listening_port"].get<std::string>().c_str()));
    
    
    cUDPClient.SetMessageOnReceive (&onReceive);
    cUDPClient.start();
}


void initGPIO()
{
    const Json& jsonConfig = cConfigFile.GetConfigJSON();
    if (!jsonConfig.contains("enable_led")) return ;
    if (jsonConfig["enable_led"].get<bool>()==true)
    {
        cLeds.init();
    }
    
     
}

void initArguments (int argc, char *argv[])
{
    int opt;
    const struct GetOptLong::option options[] = {
        {"config",         true,   0, 'c'},
        {"version",        false,  0, 'v'},
        {"help",           false,  0, 'h'},
        {0, false, 0, 0}
    };
    // adding ':' means there is extra parameter needed
    GetOptLong gopt(argc, argv, "c:vh",
                    options);

    /*
      parse command line options
     */
    while ((opt = gopt.getoption()) != -1) {
        switch (opt) {
        case 'c':
            configName = gopt.optarg;
            break;
        case 'v':
            _version();
            exit(0);
            break;
        case 'h':
            _usage();
            exit(0);
        default:
            printf("Unknown option '%c'\n", (char)opt);
            exit(1);
        }
    }
}

/**
 * initialize components
 **/
void init (int argc, char *argv[]) 
{

    initArguments (argc, argv);

    // Reading Configuration
    std::cout << std::endl << _SUCCESS_CONSOLE_BOLD_TEXT_ << "=================== " << "STARTING UAVOS COMMUNICATOR ===================" << _NORMAL_CONSOLE_TEXT_ << std::endl;

    signal(SIGINT,quit_handler);
    signal(SIGTERM,quit_handler);
	
    
    cConfigFile.InitConfigFile (configName.c_str());
    
    defineMe();
    
    initGPIO();

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
    m_scheduler.join();
	//pthread_join(m_scheduler ,NULL);
	
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
    init (argc, argv);

    while (!exit_me)
    {
       std::this_thread::sleep_for(std::chrono::seconds(1));
       
    }

}