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
#include "localConfigFile.hpp"

#include "./comm_server/andruav_unit.hpp"
#include "./comm_server/andruav_comm_server_manager.hpp"
#include "./comm_server/andruav_facade.hpp"
#include "./de_broker/de_modules_manager.hpp"
#include "./hal/gpio.hpp"
#include "./notification_module/leds.hpp"
#include "./notification_module/buzzer.hpp"
#include "./de_general_mission_planner/mission_file.hpp"
#include "./de_general_mission_planner/mission_manager_base.hpp"
#include <plog/Log.h> 
#include "plog/Initializers/RollingFileInitializer.h"

//using namespace boost;
using namespace std;


de::CConfigFile& cConfigFile = de::CConfigFile::getInstance();
de::CLocalConfigFile& cLocalConfigFile = de::CLocalConfigFile::getInstance();

de::comm::CUavosModulesManager& cUavosModulesManager = de::comm::CUavosModulesManager::getInstance();  
//hal_linux::CRPI_GPIO &cGPIO = hal_linux::CRPI_GPIO::getInstance();
notification::CLEDs &cLeds = notification::CLEDs::getInstance();
notification::CBuzzer &cBuzzer = notification::CBuzzer::getInstance();

std::thread m_scheduler;
bool exit_scheduler = false;


    
static std::string configName = "de_comm.config.module.json";
static std::string localConfigName = "de_comm.local";

// This is a timestamp used as instance unique number. if changed then communicator module knows module has restarted.
std::time_t instance_time_stamp;

void quit_handler( int sig );
/**
 * @brief display version info
 * 
 */


void _version (void)
{
    std::cout << std::endl << _SUCCESS_CONSOLE_BOLD_TEXT_ "Drone-Engage Communicator Server version " << _INFO_CONSOLE_TEXT << version_string << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #ifdef DEBUG
    std::cout << _INFO_CONSOLE_TEXT << "BUILD DATE:" << _LOG_CONSOLE_BOLD_TEXT << __DATE__ << " --- " << __TIME__ << std::endl;
    #endif
}

void _versionOnly (void)
{ // used by script to autoupdate. Do Not change this function.
    std::cout << version_string << std::endl;
}

/**
 * @brief display help for -h command argument.
 * 
 */
void _usage(void)
{
    _version ();
    std::cout << std::endl << _INFO_CONSOLE_TEXT "Options" << _NORMAL_CONSOLE_TEXT_ << std::ends;
    std::cout << std::endl << _INFO_CONSOLE_TEXT "\t--config:          -c ./config.json   default [./de_comm.config.module.json]" << _NORMAL_CONSOLE_TEXT_ << std::ends;
    std::cout << std::endl << _INFO_CONSOLE_TEXT "\t--bconfig:         -b ./bconfig.json  default [./de_comm.local]" << _NORMAL_CONSOLE_TEXT_ << std::ends;
    std::cout << std::endl << _INFO_CONSOLE_TEXT "\t--version:         -v" << _NORMAL_CONSOLE_TEXT_ << std::endl;
}
int test_counter =0;

/**
 * @brief main loop function.
 * 
 */
void scheduler ()
{
    const int every_sec_1  =  10;
    const int every_sec_5  =  50;
    const int every_sec_10 = 100;
    const int every_sec_15 = 150;

    de::andruav_servers::CAndruavFacade& andruav_facade = de::andruav_servers::CAndruavFacade::getInstance();
    
    uint64_t hz_10 = 0;
    de::STATUS &status = de::STATUS::getInstance();

    while (!exit_scheduler)
    {
        hz_10++;
        
        bool status_is_online = de::andruav_servers::CAndruavCommServerManager::getInstance().isOnline();
        status.is_online(status_is_online);

        cBuzzer.update();
        
        if (hz_10 % every_sec_1 == 0)
        {
            cLeds.update();
            if (test_counter % 10)
            {
                if (status.is_online())
                {

                }
                else
                {
                    
                }
            }
        }

        if (hz_10 % every_sec_5 == 0)
        {
            cUavosModulesManager.handleDeadModules(); //TODO: Why when online only ??
            
            de::CConfigFile &cConfigFile = de::CConfigFile::getInstance();
            const bool updated = cConfigFile.fileUpdated();
            if (updated) {
                
            }

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
                #ifdef DDEBUG
                    std::cout  << "get_throttled:" << "NOT RPI" << std::endl;
                #endif
            }
            
        }

        if (hz_10 % every_sec_10 == 0)
        {
            // Leave thie the last part in this code block.
            if (status.is_online())
            {
                andruav_facade.API_sendID("");
            }
        }

        if (hz_10 % every_sec_15 == 0)
        {
        }
        
        try {
            std::this_thread::sleep_for(std::chrono::microseconds(100000)); // 10Hz
        } catch (const std::system_error& e) {
             std::cerr << "Error joining thread Schedule: " << e.what() << std::endl;
                return ; //exit 
        } 
    }

    return ;
}


void initScheduler()
{
    m_scheduler = std::thread (scheduler);
}

void initLogger()
{
    const Json_de& jsonConfig = cConfigFile.GetConfigJSON();
    
    if ((jsonConfig.contains("logger_enabled") == false) || (jsonConfig["logger_enabled"].get<bool>()==false))
    {
        std::cout  << _LOG_CONSOLE_BOLD_TEXT << "Logging " << _ERROR_CONSOLE_BOLD_TEXT_ << "DISABLED" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
        return ;
    }

    std::string log_filename = "log";
    bool debug_log = false; 
    if (jsonConfig.contains("logger_debug"))
    {
        debug_log = jsonConfig["logger_debug"].get<bool>();
    }

    std::cout  << _LOG_CONSOLE_BOLD_TEXT << "Logging " << _SUCCESS_CONSOLE_BOLD_TEXT_ << "ENABLED" << _NORMAL_CONSOLE_TEXT_ <<  std::endl;

        

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream log_filename_final;
    log_filename_final <<  "./logs/log_" << std::put_time(&tm, "%d-%m-%Y_%H-%M-%S") << ".log";
    mkdir("./logs/",0777);

    std::cout  << _LOG_CONSOLE_BOLD_TEXT << "Logging to " << _INFO_CONSOLE_TEXT << log_filename << _LOG_CONSOLE_BOLD_TEXT << " detailed:" << _INFO_CONSOLE_TEXT << log_filename_final.str() <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
    auto log_level = debug_log==true?plog::debug:plog::info;

    plog::init(log_level, log_filename_final.str().c_str()); 

    PLOG(plog::info) << "Drone-Engage Communicator Server version " << version_string; 
}

void defineMe()
{
    const Json_de& jsonConfig = cConfigFile.GetConfigJSON();
    de::CAndruavUnitMe& m_andruavMe = de::CAndruavUnitMe::getInstance();
    de::ANDRUAV_UNIT_INFO&  unit_info = m_andruavMe.getUnitInfo();
    
    std::string party_id = cLocalConfigFile.getStringField("party_id");
    if (party_id=="")
    {
        party_id = std::to_string((get_time_usec() & 0xFFFFFF));
        cLocalConfigFile.addStringField("party_id",party_id.c_str());
        cLocalConfigFile.apply();
    }
    std::string module_key = cLocalConfigFile.getStringField("module_key");
    if (module_key=="")
    {
        
        module_key = std::to_string(get_time_usec());
        cLocalConfigFile.addStringField("module_key",module_key.c_str());
        cLocalConfigFile.apply();
    }

    std::string unit_name = cLocalConfigFile.getStringField("unitID");
    if (unit_name=="") 
    {
        unit_name = jsonConfig["unitID"].get<std::string>();
        cLocalConfigFile.addStringField("unitID",unit_name.c_str());
        cLocalConfigFile.apply();
    }

    std::string unit_description = cLocalConfigFile.getStringField("unitDescription");
    if (unit_description=="") 
    {
        unit_description = jsonConfig["unitDescription"].get<std::string>();
        cLocalConfigFile.addStringField("unitDescription",unit_description.c_str());
        cLocalConfigFile.apply();
    }

    if (validateField(jsonConfig, "unit_type",Json_de::value_t::string))
    {
        std::string vehicle_type  = jsonConfig["unit_type"].get<std::string>();
        if (str_tolower(vehicle_type)=="control_unit")
        {
            unit_info.vehicle_type = de::ANDRUAV_UNIT_TYPE::CONTROL_UNIT;
        }
    }
    
    std::cout  << _LOG_CONSOLE_BOLD_TEXT << "Party Id " << _INFO_CONSOLE_TEXT << party_id << _NORMAL_CONSOLE_TEXT_ <<  std::endl;
    std::cout  << _LOG_CONSOLE_BOLD_TEXT << "Module Key " << _INFO_CONSOLE_TEXT << module_key << _NORMAL_CONSOLE_TEXT_ <<  std::endl;
    
    
    unit_info.party_id = party_id;
    unit_info.unit_name = unit_name;
    unit_info.description = unit_description;
    unit_info.group_name = jsonConfig["groupID"].get<std::string>();
    
}






/**
 * @brief Initialize UDP connection with other modules.
 * 
 */
void initModuleManager()
{
    de::CConfigFile& cConfigFile = de::CConfigFile::getInstance();
    const Json_de& jsonConfig = cConfigFile.GetConfigJSON();
    de::CLocalConfigFile& cLocalConfigFile = de::CLocalConfigFile::getInstance();
    std::string module_key = cLocalConfigFile.getStringField("module_key");
    de::ANDRUAV_UNIT_INFO&  unit_info = de::CAndruavUnitMe::getInstance().getUnitInfo();
    
        
    int udp_chunk_size = DEFAULT_UDP_DATABUS_PACKET_SIZE;
    
    if (validateField(jsonConfig, "s2s_udp_packet_size",Json_de::value_t::string)) 
    {
        udp_chunk_size = std::stoi(jsonConfig["s2s_udp_packet_size"].get<std::string>());
    }
    else
    {
        std::cout << _LOG_CONSOLE_BOLD_TEXT << "WARNING:" << _INFO_CONSOLE_TEXT << " MISSING FIELD " << _ERROR_CONSOLE_BOLD_TEXT_ << "s2s_udp_packet_size " <<  _INFO_CONSOLE_TEXT << "is missing in config file. default value " << _ERROR_CONSOLE_BOLD_TEXT_  << std::to_string(DEFAULT_UDP_DATABUS_PACKET_SIZE) <<  _INFO_CONSOLE_TEXT <<  " is used." << _NORMAL_CONSOLE_TEXT_ << std::endl;    
    }

    cUavosModulesManager.defineModule( MODULE_CLASS_COMM, 
                        jsonConfig["module_id"],
                        cLocalConfigFile.getStringField("module_key"),
                        version_string,
                        unit_info.party_id,
                        jsonConfig["groupID"].get<std::string>());

    cUavosModulesManager.init(jsonConfig["s2s_udp_listening_ip"].get<std::string>().c_str() ,
                    std::stoi(jsonConfig["s2s_udp_listening_port"].get<std::string>().c_str()),
                    udp_chunk_size);
    
}


void initGPIO()
{
    const Json_de& jsonConfig = cConfigFile.GetConfigJSON();

    if ((jsonConfig.contains("led_pins_enabled")) && (jsonConfig["led_pins_enabled"].get<bool>()==false))
    {
        std::cout  << _INFO_CONSOLE_TEXT << "LEDs pins \"led_pins\" are not defined. Notification will be " << _ERROR_CONSOLE_BOLD_TEXT_ << "DISABLED" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return ;
    }


    cLeds.init();
    
     

    if ((jsonConfig.contains("buzzer_pins_enabled")) && (jsonConfig["buzzer_pins_enabled"].get<bool>()==false))
    {
        std::cout  << _INFO_CONSOLE_TEXT << "Buzzer pins \"buzzer_pins\" are not defined. Notification will be " << _ERROR_CONSOLE_BOLD_TEXT_ << "DISABLED" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return ;
    }

    cBuzzer.init();
    
}

void initArguments (int argc, char *argv[])
{
    int opt;
    const struct GetOptLong::option options[] = {
        {"config",         true,   0, 'c'},
        {"bconfig",        true,   0, 'b'},
        {"version",        false,  0, 'v'},
        {"versiononly",    false,  0, 'o'},
        {"help",           false,  0, 'h'},
        {0, false, 0, 0}
    };
    // adding ':' means there is extra parameter needed
    GetOptLong gopt(argc, argv, "c:b:voh",
                    options);

    /*
      parse command line options
     */
    while ((opt = gopt.getoption()) != -1) {
        switch (opt) {
        case 'c':
            configName = gopt.optarg;
            break;
        case 'b':
            localConfigName = gopt.optarg;
            break;
        case 'v':
            _version();
            exit(0);
            break;
        case 'o':
            _versionOnly();
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

void readSavedMission()
{
const std::string plan = de::mission::CMissionFile::getInstance().readMissionFile("de_plan.json");
    if (plan != "")
    {
        const Json_de json_plan = Json_de::parse(plan);
        de::mission::CMissionManagerBase::getInstance().extractPlanModule(json_plan);
    }   
    else
    {
        std::cout << _INFO_CONSOLE_TEXT << "No saved mission file found." << _NORMAL_CONSOLE_TEXT_ << std::endl;
    }
}

/**
 * initialize components
 **/
void init (int argc, char *argv[]) 
{

    instance_time_stamp = std::time(nullptr);

    initArguments (argc, argv);

    // Reading Configuration
    std::cout << std::endl << _SUCCESS_CONSOLE_BOLD_TEXT_ << "=================== " << "STARTING DroneEngage COMMUNICATOR ===================" << _NORMAL_CONSOLE_TEXT_ << std::endl;

    signal(SIGINT,quit_handler);
    signal(SIGTERM,quit_handler);
	
    
    cConfigFile.initConfigFile (configName.c_str());
    cLocalConfigFile.initConfigFile (localConfigName.c_str());
    
    _version();
    
    #ifdef DEBUG
    std::cout << _INFO_CONSOLE_TEXT << "BUILD DATE:" << _LOG_CONSOLE_BOLD_TEXT << __DATE__ << " --- " << __TIME__ << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    std::cout << _INFO_CONSOLE_TEXT << std::asctime(std::localtime(&instance_time_stamp)) << instance_time_stamp << _LOG_CONSOLE_BOLD_TEXT << " seconds since the Epoch" << _NORMAL_CONSOLE_TEXT_  << std::endl;

    initLogger();

    defineMe();
    
    initGPIO();

    initScheduler();

    initModuleManager();
    readSavedMission();
            
}


void loop() {
    de::andruav_servers::CAndruavCommServer& andruav_server = 
        de::andruav_servers::CAndruavCommServer::getInstance();

    de::andruav_servers::CAndruavCommServerLocal& andruav_server_local = 
        de::andruav_servers::CAndruavCommServerLocal::getInstance();

    const Json_de& jsonConfig = cConfigFile.GetConfigJSON();

    std::string m_local_comm_server_ip = "";
    int m_local_comm_server_port = 0;

    if ((validateField(jsonConfig,"local_comm_server_ip", Json_de::value_t::string))
    && (validateField(jsonConfig,"local_comm_server_port", Json_de::value_t::number_unsigned)))
    {
        andruav_server_local.isLocalCommServer(true);
        m_local_comm_server_ip = jsonConfig["local_comm_server_ip"].get<std::string>();
        m_local_comm_server_port = jsonConfig["local_comm_server_port"].get<int>();

        andruav_server_local.start(m_local_comm_server_ip, m_local_comm_server_port, "my_key");
    }

    

    bool is_warning_notified = false; // Flag to track if we've encountered ignore_original_comm_server = true

    while (!de::STATUS::getInstance().m_exit_me) {
        
        bool shouldIgnoreAuth = false;
        if (validateField(jsonConfig, "ignore_original_comm_server", Json_de::value_t::boolean)) {
            shouldIgnoreAuth = jsonConfig["ignore_original_comm_server"].get<bool>();
        }

        if (shouldIgnoreAuth) {
            if (!is_warning_notified) {
                std::cout << _INFO_CONSOLE_BOLD_TEXT << "WARNING:: ignore_original_comm_server is true. Main communication server channel will NOT be started." << _NORMAL_CONSOLE_TEXT_ << std::endl;
                is_warning_notified = true;
            }
            // Do not start the server if ignore_original_comm_server is true
        } else {
            andruav_server.start();
            is_warning_notified = false; // Reset the flag if ignore_original_comm_server is false or absent
        } 

        if (andruav_server_local.isLocalCommServer())
        {
            andruav_server_local.start();
        }

        try {
            std::this_thread::sleep_for(std::chrono::seconds(1)); 
        } catch (const std::system_error& e) {
            std::cerr << "Sleep interrupted by signal: " << e.what() << std::endl;
            return ; //exit
        }
        
    }
}


void uninit ()
{

    
    de::STATUS::getInstance().m_exit_me = true;
    exit_scheduler = true;
    

    cLeds.uninit();
    cUavosModulesManager.uninit();
    de::andruav_servers::CAndruavCommServerManager::getInstance().uninit(true);
    
    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: Unint" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    
    try {
    if (m_scheduler.joinable()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Short wait
        m_scheduler.join();
    }
    } catch (const std::system_error& e) {
        std::cerr << "Error joining thread2: " << e.what() << std::endl;
        // Handle the exception gracefully (e.g., log the error, attempt recovery)
    }

    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: Unint_after Stop" << _NORMAL_CONSOLE_TEXT_ << std::endl;
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
            std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: quit_handler" << std::to_string(error)<< _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif

    }

    exit(0);
}




int main (int argc, char *argv[])
{
    #ifdef DDEBUG
        std::cout << _INFO_CONSOLE_BOLD_TEXT << " ========================== D-DEBUG ENABLED =========================="   << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
	
    init (argc, argv);

    loop();

}