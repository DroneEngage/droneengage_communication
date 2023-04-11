

#include <cstdlib>
#include <string>
#include <iostream>

#include <thread>
#define BOOST_BEAST_ALLOW_DEPRECATED

#include <plog/Log.h> 
#include "plog/Initializers/RollingFileInitializer.h"


#include "../helpers/colors.hpp"
#include "../helpers/helpers.hpp"
#include "../helpers/util_rpi.hpp"
#include "../messages.hpp"
#include "../configFile.hpp"
#include "../localConfigFile.hpp"
#include "andruav_auth.hpp"
#include "andruav_unit.hpp"
#include "../uavos/uavos_modules_manager.hpp"
#include "andruav_comm_server.hpp"
#include "andruav_facade.hpp"

// Based on Below Model
// https://www.boost.org/doc/libs/develop/libs/beast/example/websocket/client/async-ssl/websocket_client_async_ssl.cpp

// ------------------------------------------------------------------------------
//  Pthread Starter Helper Functions
// ------------------------------------------------------------------------------


using namespace uavos::andruav_servers;

pthread_t m_watch_dog3;

void* uavos::andruav_servers::startWatchDogThread3(void *args)
{
    
    std::cout <<_INFO_CONSOLE_TEXT << "Restarting Sockets has been Engaged..." << _NORMAL_CONSOLE_TEXT_ << std::endl;

    uavos::andruav_servers::CAndruavCommServer& andruav_server = uavos::andruav_servers::CAndruavCommServer::getInstance();
    
    andruav_server.uninit(true);
    
    andruav_server.start();

    return NULL;
}

void* uavos::andruav_servers::startWatchDogThread2(void *args)
{
    /**
     * @brief This function mainly detects if communication is idle and there is no disconnection has been detected.
     * It does that by sending PING backed to Communication Server and geting the reply.
     * Receiving a ping message will result in incrementing @link.getLastTimeAccess()
     * A delay for 2 seconds (default) will cause a restart.
     */
    
    uavos::andruav_servers::CAndruavCommServer& andruav_server = uavos::andruav_servers::CAndruavCommServer::getInstance();
    UNUSED (andruav_server);

    // ping every 1500 ms ass default
    uint32_t ping_server_rate_in_us = 1500 * 1000l; 
    uint32_t max_allowed_ping_delay_in_us = 5000;

    uavos::CConfigFile& cConfigFile = uavos::CConfigFile::getInstance();
    const Json& jsonConfig = cConfigFile.GetConfigJSON();
    if (validateField(jsonConfig,"ping_server_rate_in_ms", Json::value_t::number_unsigned))
    {
        ping_server_rate_in_us = (uint32_t) jsonConfig["ping_server_rate_in_ms"].get<int>()  * 1000l;
    }
    if (validateField(jsonConfig,"max_allowed_ping_delay_in_ms", Json::value_t::number_unsigned))
    {
        max_allowed_ping_delay_in_us = (uint32_t) jsonConfig["max_allowed_ping_delay_in_ms"].get<int>() * 1000l;
    }
    
        
    while (true)
    {
            if (andruav_server.shouldExit()) 
            {
                return NULL;
            }

            if ((andruav_server.getLastTimeAccess()!=0)
                &&                                                       
                ((get_time_usec() - andruav_server.getLastTimeAccess()) > max_allowed_ping_delay_in_us)
                )
                {
                    if (andruav_server.getStatus() == SOCKET_STATUS_REGISTERED)
                    {  
                        pthread_create( &m_watch_dog3, NULL, &uavos::andruav_servers::startWatchDogThread3, NULL );
                        return NULL;
                    }
                }

            
                #ifdef DEBUG
                    std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: andruav_server.shouldExit() = false diff="  << (get_time_usec() - andruav_server.getLastTimeAccess()) << _NORMAL_CONSOLE_TEXT_ << std::endl;
                #endif

            if (andruav_server.getStatus() != SOCKET_STATUS_FREASH)
            {
                andruav_server.API_pingServer();
            }

            usleep(ping_server_rate_in_us); 
    }

	return NULL;
}

void* uavos::andruav_servers::startWatchDogThread(void *args)
{
	
    uavos::andruav_servers::CAndruavCommServer& andruav_server = uavos::andruav_servers::CAndruavCommServer::getInstance();
        
    while (true)
    {
        // * note that connect does not return when it successfully connects
        andruav_server.connect(); 

        for (int i=0;i<50;++i)
        {
            if (andruav_server.shouldExit()) 
            {
                #ifdef DEBUG
                    std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: andruav_server.shouldExit() == true" << _NORMAL_CONSOLE_TEXT_ << std::endl;
                #endif

                return NULL;
            }
	        #ifdef DEBUG
                    std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: andruav_server.shouldExit() == false & i = " << i << _NORMAL_CONSOLE_TEXT_ << std::endl;
            #endif

            usleep(10000); 
        }

        
    }

	return NULL;
}


void uavos::andruav_servers::CAndruavCommServer::start ()
{
    m_exit = false;
    m_lasttime_access = 0;
    int result = pthread_create( &m_watch_dog, NULL, &uavos::andruav_servers::startWatchDogThread, this );
    if ( result ) throw result;
    
    result = pthread_create( &m_watch_dog2, NULL, &uavos::andruav_servers::startWatchDogThread2, this );
    if ( result ) throw result;

}


/**
 * @brief Main function that connects to Andruav Authentication
 * 
 */
void uavos::andruav_servers::CAndruavCommServer::connect ()
{
    try
    {

        if (m_status == SOCKET_STATUS_CONNECTING)
        {
            PLOG(plog::info) << "Communicator Server Connection Status: SOCKET_STATUS_CONNECTING";
            return ;
        }

        if (m_status == SOCKET_STATUS_REGISTERED)
        {
            PLOG(plog::info) << "Communicator Server Connection Status: SOCKET_STATUS_REGISTERED";
            return ;
        }

        const uint64_t now_time = get_time_usec();
        
        if (m_next_connect_time > now_time)
        {
            return ;
        }

        m_next_connect_time = now_time + 10000000l; // retry after 10 sec.

        uavos::andruav_servers::CAndruavAuthenticator& andruav_auth = uavos::andruav_servers::CAndruavAuthenticator::getInstance();
        
        m_status = SOCKET_STATUS_CONNECTING;
        if (!andruav_auth.doAuthentication() || !andruav_auth.isAuthenticationOK())   
        {
            m_status = SOCKET_STATUS_ERROR;
            PLOG(plog::error) << "Communicator Server Connection Status: SOCKET_STATUS_ERROR"; 
            uavos::CUavosModulesManager::getInstance().handleOnAndruavServerConnection (m_status);
            return ;
        }
    

        std::string serial;
        if (helpers::CUtil_Rpi::getInstance().get_cpu_serial(serial)!= false)
        {
            std::cout << "Unique Key :" << serial << std::endl;
        }
        serial.append(get_linux_machine_id());

        uavos::ANDRUAV_UNIT_INFO&  unit_info = uavos::CAndruavUnitMe::getInstance().getUnitInfo();
    
        connectToCommServer(andruav_auth.m_comm_server_ip, std::to_string(andruav_auth.m_comm_server_port), andruav_auth.m_comm_server_key, unit_info.party_id);

    }

    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        PLOG(plog::error) << "Communicator Server Connection Status: " << e.what(); 
        return ;
    }
}

/**
 * @brief Connects to Andruav Communication Server
 * 
 * @param server_ip 
 * @param server_port 
 * @param key 
 * @param party_id 
 */
void uavos::andruav_servers::CAndruavCommServer::connectToCommServer (const std::string& server_ip, const std::string &server_port, const std::string& key, const std::string& party_id)
{
    try
    {
        
        m_host = std::string(server_ip);
        m_port = std::string(server_port);
        m_party_id = std::string(party_id);
        // The io_context is required for all I/O
        net::io_context ioc;

        // The SSL context is required, and holds certificates
        ssl::context ctx{ssl::context::tlsv12_client};

        // This holds the root certificate used for verification
        //load_root_certificates(ctx);
        m_url_param = "/?f=" + key + "&s=" + m_party_id;
        
        // Launch the asynchronous operation
        _cwssession = std::shared_ptr<uavos::andruav_servers::CWSSession>(new uavos::andruav_servers::CWSSession(ioc, ctx, *this));
        _cwssession.get()->run(m_host.c_str(), m_port.c_str(), m_url_param.c_str());

        // Run the I/O service. The call will return when
        // the socket is closed.
        ioc.run();

        _cwssession.reset();
        
        #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: connectToCommServer" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        PLOG(plog::error) << "Connecting to Communication Server IP (" << m_host << ") Port(" << m_port << ") PartyID (" << m_party_id << ") failed with error:" << e.what(); 
        return ;
    }
}


void uavos::andruav_servers::CAndruavCommServer::onSocketError()
{
    // reset rate...socket error handling is tacking care now of reconnection.
    m_lasttime_access = 0; 

    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: onSocketError " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    if (m_exit== true)
    {
        std::cout << _INFO_CONSOLE_TEXT << "Communication Server Connection Terminated m_exit is TRUE"  << _NORMAL_CONSOLE_TEXT_ << std::endl;
        m_status =  SOCKET_STATUS_DISCONNECTED;  
        PLOG(plog::warning) << "Communicator Server Connection Status: SOCKET_STATUS_DISCONNECTED with m_exit is TRUE"; 
    }
    else
    {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Communication Server: Socket Status Error "  << _NORMAL_CONSOLE_TEXT_ << std::endl;
        m_status = SOCKET_STATUS_ERROR;
        PLOG(plog::error) << "Communicator Server: Socket Status Error"; 
    }

    uavos::CUavosModulesManager::getInstance().handleOnAndruavServerConnection (m_status);
}

/**
 * @brief 
 * 
 * @param message first part until byte of value'0' should be XML header.
 * @param datalength 
 */
void uavos::andruav_servers::CAndruavCommServer::onBinaryMessageRecieved (const char * message, const std::size_t datalength)
{
    m_lasttime_access = get_time_usec();

    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: onBinaryMessageRecieved " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    Json jMsg;
    jMsg = Json::parse(message);
    if (!validateField(jMsg, INTERMODULE_ROUTING_TYPE, Json::value_t::string))
    {
        // bad message format
        return ;
    }

    if (!validateField(jMsg, ANDRUAV_PROTOCOL_MESSAGE_TYPE, Json::value_t::number_unsigned))
    {
        // bad message format
        return ;
    }


    if (jMsg[INTERMODULE_ROUTING_TYPE].get<std::string>().compare(CMD_TYPE_SYSTEM_MSG)==0)
    {   // System Message
        
    }
    else
    {
        if (!validateField(jMsg, ANDRUAV_PROTOCOL_SENDER, Json::value_t::string))
        {
            // bad message format
            return ;
        }

        std::string sender = jMsg[ANDRUAV_PROTOCOL_SENDER];

        const int command_type = jMsg[ANDRUAV_PROTOCOL_MESSAGE_TYPE].get<int>();
        switch (command_type)
        {
            case TYPE_AndruavMessage_RemoteExecute:
            {
                parseRemoteExecuteCommand(sender, jMsg);
            }
            break;

            default:
            {
                parseCommand(sender, command_type, jMsg);
            }
            break;
        }

        uavos::CUavosModulesManager::getInstance().processIncommingServerMessage(sender, command_type,  message, datalength, std::string());
    }
    
}
            

/**
 * @brief text message recieved from ANdruavServerComm.
 * 
 * @param jsonMessage string message in JSON format.
 */
void uavos::andruav_servers::CAndruavCommServer::onTextMessageRecieved(const std::string& jsonMessage)
{
    m_lasttime_access = get_time_usec();

    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: onMessageRecieved " << jsonMessage << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    Json jMsg;
    jMsg = Json::parse(jsonMessage);
    if (!validateField(jMsg, INTERMODULE_ROUTING_TYPE, Json::value_t::string))
    {
        // bad message format
        return ;
    }

    if (!validateField(jMsg, ANDRUAV_PROTOCOL_MESSAGE_TYPE, Json::value_t::number_unsigned))
    {
        // bad message format
        return ;
    }

    // fix: CWSSession::writeText & CWSSession::on_read spmetimes fails but with a live socket
    // setting m_status to SOCKET_STATUS_ERROR will render this socket unuseful althougfh there is a valid connection
    // so this fix to return socket to Connected state when receiving message from server.
    if (m_status == SOCKET_STATUS_ERROR)
    { // if not OK then TYPE_AndruavSystem_ConnectedCommServer will have not OK and will set it back. 
        std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "Communication Server ReConnected: Success "  << _NORMAL_CONSOLE_TEXT_ << std::endl;
        m_status = SOCKET_STATUS_REGISTERED;
        uavos::CUavosModulesManager::getInstance().handleOnAndruavServerConnection (m_status);
    }


    if (jMsg[INTERMODULE_ROUTING_TYPE].get<std::string>().compare(CMD_TYPE_SYSTEM_MSG)==0)
    {
        const int command_type = jMsg[ANDRUAV_PROTOCOL_MESSAGE_TYPE].get<int>();
        switch (command_type)
        {
            // Server replied when connection  has been established.
            case TYPE_AndruavSystem_ConnectedCommServer:
            {   
                // example onMessageRecieved {"ty":"s","mt":9007,"ms":{"s":"OK:connected:tcp:192.168.1.144:37196"}}
                Json message_cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
                if (message_cmd["s"].get<std::string>().find("OK")==0)
                {
                    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "Communication Server Connected: Success "  << _NORMAL_CONSOLE_TEXT_ << std::endl;
                    PLOG(plog::info) << "Communication Server Connected: Success ";
                    
                    m_status = SOCKET_STATUS_REGISTERED;
                    uavos::andruav_servers::CAndruavFacade::getInstance().API_requestID(std::string());
                //    uavos::andruav_servers::CAndruavFacade::getInstance().API_loadTasksByScope(ENUM_TASK_SCOPE::SCOPE_GROUP, TYPE_AndruavMessage_ExternalGeoFence);
                }
                else
                {
                    std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Communication Server Connected: Failed "  << _NORMAL_CONSOLE_TEXT_ << std::endl;
                    PLOG(plog::error) << "Communication Server Connected: Failed "; 

                    m_status = SOCKET_STATUS_ERROR;
                }
                uavos::CUavosModulesManager::getInstance().handleOnAndruavServerConnection (m_status);
            }
            break;

            case TYPE_AndruavSystem_LoadTasks:
            {
                    //TODO: Execute load tasks ... asked by server  
            }
            default:
                
                break;
        }
    }
    else
    {
        if (!validateField(jMsg, ANDRUAV_PROTOCOL_SENDER, Json::value_t::string))
        {
            // bad message format
            return ;
        }

        std::string sender = jMsg[ANDRUAV_PROTOCOL_SENDER];


        
        const int command_type = jMsg[ANDRUAV_PROTOCOL_MESSAGE_TYPE].get<int>();
        switch (command_type)
        {
            case TYPE_AndruavMessage_RemoteExecute:
            {
                parseRemoteExecuteCommand(sender, jMsg);
            }
            break;

            default:
            {
                parseCommand(sender, command_type, jMsg);
            }
            break;
        }

        uavos::CUavosModulesManager::getInstance().processIncommingServerMessage(sender, command_type,  jsonMessage.c_str(), jsonMessage.length(), std::string());
    }
}


void uavos::andruav_servers::CAndruavCommServer::parseCommand (const std::string& sender_party_id, const int& command_type, const Json& jsonMessage)
{
    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: parseCommand " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    // retreive unit or create a new one
    uavos::CAndruavUnit* unit = m_andruav_units.getUnitByName(sender_party_id);
    ANDRUAV_UNIT_INFO& unit_info = unit->getUnitInfo();
    
    // get message command details
    const Json& msg_cmd = jsonMessage.contains(ANDRUAV_PROTOCOL_MESSAGE_CMD)?jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD]:Json();
    
    // if unit is new then ask for details.
    if ((command_type!=TYPE_AndruavMessage_ID) && (unit_info.is_new == true))  
    {
        uavos::andruav_servers::CAndruavFacade::getInstance().API_requestID (sender_party_id);    
        
        /*
            DONOT add return here unless system requires more security.
            You cannot receive messages except from units that are logged so it should be secure.
            if you enable the return the following issue may happen:
                1- GCS receives ID messages from the unit.
                2- GCS send asking for mission & other info
                3- unit will ignore these messages until it receives a MSG_ID from the WEB.
                4- WebClient sending messages is stateless in general.
        */
        
        //return;
    }

    switch (command_type)
    {
        case TYPE_AndruavMessage_ID:
        {
            /*
                TYPE_AndruavMessage_ID
                GS:bool: is gcs
                UD:string: unit name
                DS:string: description
                VT:int: vehicle type

                FI:bool: useFCBIMU (optional default:false)
                
            */
            const Json command = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD];
            unit_info.vehicle_type = command["VT"].get<int>();
            unit_info.is_gcs = command["GS"].get<bool>();
            
            
            unit_info.party_id = sender_party_id;
            
            if (!validateField(command,"UD", Json::value_t::string)) return ;
            if (!validateField(command,"DS", Json::value_t::string)) return ;
            
            unit_info.unit_name = command["UD"].get<std::string>();
            unit_info.description = command["DS"].get<std::string>();
            
            if (command.contains("VR") == true) unit_info.is_video_recording = command["VR"].get<int>();
            if (command.contains("FI") == true) unit_info.use_fcb = command["FI"].get<bool>();
            if (command.contains("SD") == true) unit_info.is_shutdown = command["SD"].get<bool>();
            if (command.contains("GM") == true) unit_info.gps_mode = command["GM"].get<int>();
            if (command.contains("AR") == true) unit_info.is_armed = command["AR"].get<bool>();
            if (command.contains("FL") == true) unit_info.is_flying = command["FL"].get<bool>();
            if (command.contains("AP") == true) unit_info.autopilot = command["AP"].get<int>();
            if (command.contains("FM") == true) unit_info.flying_mode = command["FM"].get<int>();
            if (command.contains("B") == true) unit_info.is_gcs_blocked = command["B"].get<bool>();
            if (command.contains("x") == true) unit_info.is_flashing = command["x"].get<bool>();
            if (command.contains("y") == true) unit_info.is_whisling = command["y"].get<bool>();
            if (command.contains("b") == true) unit_info.is_tracking_mode = command["b"].get<bool>();
            if (command.contains("z") == true) unit_info.flying_last_start_time = command["z"].get<long long>();
            if (command.contains("a") == true) unit_info.flying_total_duration = command["a"].get<long long>();
            if (command.contains("p") == true) unit_info.permission = command["p"].get<std::string>();
            
            
            if (command.contains("C") == true) unit_info.manual_TX_blocked_mode = command["C"].get<int>();
            
            if (command.contains("o") == true) unit_info.swarm_leader_formation = command["o"].get<int>();
            if (command.contains("q") == true) unit_info.swarm_leader_I_am_following = command["q"].get<std::string>();
            
            unit_info.last_access_time = get_time_usec();
            if (unit_info.is_new == true)
            {
                unit_info.is_new = false;
            }
        }
        break;

        case TYPE_AndruavMessage_Unit_Name:
        {
            /*
                TYPE_AndruavMessage_Unit_Name
                UN:string: unit name
                DS:string: unit description
                PR: true/false [optional]
            */

            const Json command = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD];
            
            if (!validateField(command,"UN", Json::value_t::string)) return ;
            if (!validateField(command,"DS", Json::value_t::string)) return ;

            uavos::CAndruavUnitMe& m_andruavMe = uavos::CAndruavUnitMe::getInstance();
            uavos::ANDRUAV_UNIT_INFO&  unit_info = m_andruavMe.getUnitInfo();

            unit_info.unit_name   = command["UN"].get<std::string>();
            unit_info.description = command["DS"].get<std::string>();

            uavos::CLocalConfigFile& cLocalConfigFile = uavos::CLocalConfigFile::getInstance();
            cLocalConfigFile.addStringField("unitID",unit_info.unit_name.c_str());
            cLocalConfigFile.apply();
            cLocalConfigFile.addStringField("unitDescription",unit_info.description.c_str());
            cLocalConfigFile.apply();
            if ((command.contains("PR") == true) && (command["PR"].get<bool>() == true))
            {
                const std::time_t instance_time_stamp = std::time(nullptr);
                const std::string party_id = std::to_string(instance_time_stamp);
                cLocalConfigFile.addStringField("party_id",party_id.c_str());
                cLocalConfigFile.apply();
                /*
                  Do not change party_id this will make it unstable because CommunicationServer expects current partyID
                  Change wll take effective after reboot or server disconnection.
                */
                //unit_info.party_id = party_id;   << do not uncomment.
            }
            uavos::andruav_servers::CAndruavFacade::getInstance().API_sendID(sender_party_id);
   
        }
        break;
    }

}


void uavos::andruav_servers::CAndruavCommServer::parseRemoteExecuteCommand (const std::string& sender_party_id, const Json& jsonMessage)
{
    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: parseRemoteExecuteCommand " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    const Json& msg_cmd = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD];
    
    if (!msg_cmd.contains("C")) return ;

    int remote_execute_command = msg_cmd["C"];

    uavos::CAndruavUnit* unit = m_andruav_units.getUnitByName(sender_party_id);
    ANDRUAV_UNIT_INFO& unit_info = unit->getUnitInfo();
    
    if ((unit_info.is_new == true) &&(remote_execute_command!=TYPE_AndruavMessage_ID)) 
    {
        uavos::andruav_servers::CAndruavFacade::getInstance().API_requestID (sender_party_id);    // ask for identification in return.      
        /*
            DONOT add return here unless system requires more security.
            You cannot receive messages except from units that are logged so it should be secure.
            if you enable the return the following issue may happen:
                1- GCS receives ID messages from the unit.
                2- GCS send asking for mission & other info
                3- unit will ignore these messages until it receives a MSG_ID from the WEB.
                4- WebClient sending messages is stateless in general.
        */
       //return ;
    }

    switch (remote_execute_command)
    {
        case TYPE_AndruavMessage_ID:
        {
            uavos::andruav_servers::CAndruavFacade::getInstance().API_sendID(sender_party_id);
        }
        break;

        case TYPE_AndruavMessage_CameraList:
        {
            uavos::andruav_servers::CAndruavFacade::getInstance().API_sendCameraList (true, sender_party_id);
        }
        break;

        case RemoteCommand_STREAMVIDEO:
        {
            if (!validateField(msg_cmd, "Act", Json::value_t::boolean))
            {
                // bad message format
                return ;
            }
            if (msg_cmd["Act"].get<bool>()==true)
            {
                uavos::andruav_servers::CAndruavFacade::getInstance().API_sendCameraList (true, sender_party_id);
            }
        }
		break;

        case RemoteCommand_TELEMETRYCTRL:
        {
            if (!validateField(msg_cmd, "Act", Json::value_t::number_unsigned))
            {
                // bad message format
                return ;
            }
            const int request_type = msg_cmd["Act"].get<int>();
            if (request_type != CONST_TELEMETRY_ADJUST_RATE) return ;
            
            int streaming_level = -1;
            if (!validateField(msg_cmd, "LVL", Json::value_t::number_unsigned))
            {
                // bad message format
                return ;
            }
            streaming_level = msg_cmd["LVL"].get<int>();
            uavos::STATUS& status = uavos::STATUS::getInstance();
            status.streaming_level(streaming_level);
        }
        break;
					
    }
}
            

void uavos::andruav_servers::CAndruavCommServer::uninit(const bool exit_mode)
{
    m_lasttime_access = 0;

    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: uninit " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    PLOG(plog::info) << "uninit initiated."; 
        
    m_exit = exit_mode;
    
    _cwssession.get()->close();
    
    struct timespec ts;
           int s;

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        exit(0);
    }
    ts.tv_sec += 10;

    s = pthread_timedjoin_np(m_watch_dog, NULL, &ts);
    if (s != 0) {
        exit(0);
    }

    #ifdef DEBUG
        std::cout << __PRETTY_FUNCTION__ <<  _LOG_CONSOLE_TEXT << "DEBUG: m_watch_dog 1" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        exit(0);
    }
    ts.tv_sec += 10;

    s = pthread_timedjoin_np(m_watch_dog2, NULL, &ts);
    if (s != 0) {
        exit(0);
    }
    #ifdef DEBUG
        std::cout << __PRETTY_FUNCTION__ <<  _LOG_CONSOLE_TEXT << "DEBUG: m_watch_dog 2" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    // // wait for exit
	// pthread_join(m_watch_dog ,NULL);
    // #ifdef DEBUG
    //     std::cout << __PRETTY_FUNCTION__ <<  _LOG_CONSOLE_TEXT << "DEBUG: m_watch_dog 1" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    // #endif

    // pthread_join(m_watch_dog2 ,NULL);
    // #ifdef DEBUG
    //     std::cout << __PRETTY_FUNCTION__ <<  _LOG_CONSOLE_TEXT << "DEBUG: m_watch_dog 2" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    // #endif

	
    PLOG(plog::info) << "uninit finished."; 
    
    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: uninit OUT " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
}


void uavos::andruav_servers::CAndruavCommServer::API_pingServer()
{
    Json message =  { 
        {"t", get_time_usec()}
    };


    API_sendSystemMessage(TYPE_AndruavSystem_Ping, message);
}

void uavos::andruav_servers::CAndruavCommServer::API_sendSystemMessage(const int command_type, const Json& msg) const 
{
    if (m_status == SOCKET_STATUS_REGISTERED)  
    {
        Json json_msg  = this->generateJSONSystemMessage (command_type, msg);
        _cwssession.get()->writeText(json_msg.dump());
    } 
}
            

/**
 * @details Sends Andruav Command to Communication Server
 *  *_GCS_: broadcast to GCS.
 *  *_AGN_: broadcast to vehicles only.
 *  *_GD_: broadcast to all..
 *  *null: means send to all units if sender is GCS, and if sender is drone means send to all GCS.
 * @param target_name party_id of a target or can be null or _GD_, _AGN_, _GCS_
 * @param command_type 
 * @param msg 
 */
void uavos::andruav_servers::CAndruavCommServer::API_sendCMD (const std::string& target_name, const int command_type, const Json& msg)
{
    static std::mutex g_i_mutex; 

    const std::lock_guard<std::mutex> lock(g_i_mutex);
    
    std::string message_routing;
    if (target_name.empty() == false)
    {  // BUG HERE PLease ensure that it sends ind.
        message_routing = CMD_COMM_INDIVIDUAL;
    }
    else
    {
        message_routing = CMD_COMM_GROUP;
    }

    if (m_status == SOCKET_STATUS_REGISTERED)  
    {
        Json json_msg  = this->generateJSONMessage (message_routing, m_party_id, target_name, command_type, msg);
        _cwssession.get()->writeText(json_msg.dump());

        // #ifdef DEBUG
        // std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "API_sendCMD " << json_msg.dump() << _NORMAL_CONSOLE_TEXT_ << std::endl;
        // #endif
    } 
}



/**
 * @details Sends Andruav Command to Communication Server
 *  *_GCS_: broadcast to GCS.
 *  *_AGN_: broadcast to vehicles only.
 *  *_GD_: broadcast to all..
 *  *null: means send to all units if sender is GCS, and if sender is drone means send to all GCS.
 * @param target_party_id party_id of a target or can be null or _GD_, _AGN_, _GCS_
 * @param command_type 
 * @param bmsg 
 * @param bmsg_length
 */
void uavos::andruav_servers::CAndruavCommServer::API_sendBinaryCMD (const std::string& target_party_id, const int command_type, const char * bmsg, const int bmsg_length, const Json& message_cmd)
{
    static std::mutex g_i_mutex; 

    const std::lock_guard<std::mutex> lock(g_i_mutex);
    
    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "API_sendCMD " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    std::string message_routing;
    if (target_party_id.empty() == false)
    {
        message_routing = CMD_COMM_INDIVIDUAL;
    }
    else
    {
        message_routing = CMD_COMM_GROUP;
    }

    if (m_status == SOCKET_STATUS_REGISTERED)  
    {
        Json json  = this->generateJSONMessage (message_routing, m_party_id, target_party_id, command_type, message_cmd);
        std::string json_msg = json.dump();
        char * msg_ptr = new char[json_msg.length() + 1 + bmsg_length];
        std::unique_ptr<char []> msg = std::unique_ptr<char []> (msg_ptr);
        strcpy(msg_ptr,json_msg.c_str());
        msg_ptr[json_msg.length()] = 0;
        memcpy(&msg[json_msg.length()+1], bmsg, bmsg_length);


        _cwssession.get()->writeBinary(msg_ptr, json_msg.length() + 1 + bmsg_length);
        
        msg.release();
        // #ifdef DEBUG
        // std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "API_sendCMD " << jmsg.dump() << _NORMAL_CONSOLE_TEXT_ << std::endl;
        // #endif
    } 
}

/**
 * @brief 
 * 
 * @param message_routing @link CMD_COMM_GROUP @endlink, @link CMD_COMM_INDIVIDUAL @endlink
 * @param sender_name 
 * @param target_party_id  single target except for the following
 * *_GD_* all GCS
 * *_AGN_* all agents
 * @param messageType 
 * @param message 
 * @return Json 
 */
Json uavos::andruav_servers::CAndruavCommServer::generateJSONMessage (const std::string& message_routing, const std::string& sender_name, const std::string& target_party_id, const int messageType, const Json& message) const
{

    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "generateJSONMessage " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    Json jMsg;
    jMsg[INTERMODULE_ROUTING_TYPE] = message_routing;
    jMsg[ANDRUAV_PROTOCOL_SENDER] = sender_name;
    if (!target_party_id.empty())
    {
        jMsg[ANDRUAV_PROTOCOL_TARGET_ID] = target_party_id;
    }
    else
    {
        // Inconsistent packet.... but dont enforce global packet for security reasons.
        //jMsg[INTERMODULE_ROUTING_TYPE] = CMD_COMM_GROUP; // enforce group if party id is null.
    }
    jMsg[ANDRUAV_PROTOCOL_MESSAGE_TYPE] = messageType;
    jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD] = message;
    

    return jMsg;
}


Json uavos::andruav_servers::CAndruavCommServer::generateJSONSystemMessage (const int messageType, const Json& message) const
{

    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "generateJSONMessage " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    Json jMsg;
    jMsg[INTERMODULE_ROUTING_TYPE]      = CMD_COMM_SYSTEM;
    jMsg[ANDRUAV_PROTOCOL_MESSAGE_TYPE] = messageType;
    jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD]  = message;
    

    return jMsg;
}

