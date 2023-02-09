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
#include <vector>
#include <thread>

#include <getopt.h>

#include "status.hpp"

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
#include "./hal/gpio.hpp"
#include "./notification_module/leds.hpp"
#include "./notification_module/buzzer.hpp"

#include <plog/Log.h> 
#include "plog/Initializers/RollingFileInitializer.h"

using namespace boost;
using namespace std;


uavos::CConfigFile& cConfigFile = uavos::CConfigFile::getInstance();
uavos::comm::CUDPCommunicator& cUDPClient = uavos::comm::CUDPCommunicator::getInstance();  
uavos::CUavosModulesManager& cUavosModulesManager = uavos::CUavosModulesManager::getInstance();  

//hal_linux::CRPI_GPIO &cGPIO = hal_linux::CRPI_GPIO::getInstance();
notification::CLEDs &cLeds = notification::CLEDs::getInstance();
notification::CBuzzer &cBuzzer = notification::CBuzzer::getInstance();

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
    const int every_sec_1  =  10;
    const int every_sec_5  =  50;
    const int every_sec_10 = 100;
    const int every_sec_15 = 150;

    uavos::andruav_servers::CAndruavFacade& andruav_facade = uavos::andruav_servers::CAndruavFacade::getInstance();
    
    uint64_t hz_10 = 0;
    uavos::STATUS &status = uavos::STATUS::getInstance();

    while (!exit_scheduler)
    {
        hz_10++;
        
        status.is_online(uavos::andruav_servers::CAndruavCommServer::getInstance().getStatus()==SOCKET_STATUS_REGISTERED);

        cLeds.update();
        cBuzzer.update();
        
        if (hz_10 % every_sec_1 == 0)
        {
            // if (andruav_server.getStatus() == SOCKET_STATUS_REGISTERED)
            // {
            //     andruav_server.API_sendID("");
            // }
        }

        if (hz_10 % every_sec_5 == 0)
        {
            cUavosModulesManager.handleDeadModules(); //TODO: Why when online only ??
            
            if (helpers::CUtil_Rpi::getInstance().get_rpi_model() != -1)
            {
                uint32_t cpu_status=0;
                // check status https://www.raspberrypi.com/documentation/computers/os.html#vcgencmd    
                if (helpers::CUtil_Rpi::getInstance().get_throttled(cpu_status))
                {
                    #ifdef DEBUG
                        std::cout  << "get_cpu_throttled: " << std::to_string(cpu_status) << std::endl;
                    #endif
                }

                uint32_t cpu_temprature=0;
                if (helpers::CUtil_Rpi::getInstance().get_cpu_temprature(cpu_temprature))
                {
                    #ifdef DEBUG
                        std::cout  << "get_cpu_temprature: " << std::to_string(cpu_temprature) << std::endl;
                    #endif
                }
            }
            else
            {
                #ifdef DEBUG
                    std::cout  << "get_throttled:" << "NOT RPI" << std::endl;
                #endif
            }
            
        }

        if (hz_10 % every_sec_10 == 0)
        {
            if (status.is_online())
            {
                andruav_facade.API_sendID("");
            }
        }

        if (hz_10 % every_sec_15 == 0)
        {
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

}


void initScheduler()
{
    m_scheduler = std::thread (scheduler);
}

void initLogger()
{
    const Json& jsonConfig = cConfigFile.GetConfigJSON();
    
    if ((jsonConfig.contains("logger_enabled") == false) || (jsonConfig["logger_enabled"].get<bool>()==false))
    {
        std::cout  << _LOG_CONSOLE_TEXT_BOLD_ << "Logging is " << _ERROR_CONSOLE_BOLD_TEXT_ << "DISABLED" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
        return ;
    }

    std::string log_filename = "log";
    bool debug_log = false; 
    if (jsonConfig.contains("logger_file_prfix"))
    {
        log_filename = jsonConfig["logger_file_prfix"].get<std::string>();
    }
    if (jsonConfig.contains("logger_debug"))
    {
        debug_log = jsonConfig["logger_debug"].get<bool>();
    }

    std::cout  << _LOG_CONSOLE_TEXT_BOLD_ << "Logging is " << _SUCCESS_CONSOLE_BOLD_TEXT_ << "ENABLED" << _NORMAL_CONSOLE_TEXT_ <<  std::endl;

    std::cout  << _LOG_CONSOLE_TEXT_BOLD_ << "Logging to " << _INFO_CONSOLE_TEXT << log_filename << _NORMAL_CONSOLE_TEXT_ << " detailed:" << debug_log <<  std::endl;
        

    plog::init(plog::debug, log_filename.c_str()); 

    PLOG(plog::info) << "Drone-Engage Communicator Server version " << version_string; 
    
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

    if (!jsonConfig.contains("led_pins")
    || ((jsonConfig.contains("led_pins_enabled")) && (jsonConfig["led_pins_enabled"].get<bool>()==false))
    )
    {
        std::cout  << _INFO_CONSOLE_TEXT << "LEDs pins \"led_pins\" are not defined. Notification will be " << _ERROR_CONSOLE_BOLD_TEXT_ << "DISABLED" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return ;
    }


    std::vector<notification::PORT_STATUS> led_pins;

    for (auto const& pin : jsonConfig["led_pins"])
    {
        led_pins.push_back({pin["name"].get<std::string>(),static_cast<uint8_t>(pin["gpio"].get<int>()), LED_STATUS_OFF});
    }
    
    cLeds.init(led_pins);
    
     

    if (!jsonConfig.contains("buzzer_pins")
    || ((jsonConfig.contains("buzzer_pins_enabled")) && (jsonConfig["buzzer_pins_enabled"].get<bool>()==false))
    )
    {
        std::cout  << _INFO_CONSOLE_TEXT << "Buzzer pins \"buzzer_pins\" are not defined. Notification will be " << _ERROR_CONSOLE_BOLD_TEXT_ << "DISABLED" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return ;
    }

    std::vector<notification::PORT_STATUS> buzzer_pins;

    for (auto const& pin : jsonConfig["buzzer_pins"])
    {
        buzzer_pins.push_back({pin["name"].get<std::string>(),static_cast<uint8_t>(pin["gpio"].get<int>()), GPIO_OFF});
    }
    
    cBuzzer.init(buzzer_pins);
    
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
    
    initLogger();

    defineMe();
    
    initGPIO();

    initScheduler();

    initSockets();

    connectToCommServer ();

}

void uninit ()
{

    uavos::andruav_servers::CAndruavCommServer& andruav_server = uavos::andruav_servers::CAndruavCommServer::getInstance();

    uavos::STATUS::getInstance().m_exit_me = true;
    exit_scheduler = true;
    // wait for exit
    m_scheduler.join();
	//pthread_join(m_scheduler ,NULL);
	
    cLeds.uninit();
    
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

    while (!uavos::STATUS::getInstance().m_exit_me)
    {
       std::this_thread::sleep_for(std::chrono::seconds(1));
       
    }

}