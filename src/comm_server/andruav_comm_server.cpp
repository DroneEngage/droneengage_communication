

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
#include "../de_broker/de_modules_manager.hpp"
#include "andruav_comm_server.hpp"
#include "andruav_facade.hpp"

std::thread g;
// Based on Below Model
// https://www.boost.org/doc/libs/develop/libs/beast/example/websocket/client/async-ssl/websocket_client_async_ssl.cpp

// ------------------------------------------------------------------------------
//  Pthread Starter Helper Functions
// ------------------------------------------------------------------------------


using namespace de::andruav_servers;


void de::andruav_servers::CAndruavCommServer::startWatchDogThread()
{
    static uint32_t off_count = 0;

	de::CConfigFile& cConfigFile = de::CConfigFile::getInstance();
    const Json_de& jsonConfig = cConfigFile.GetConfigJSON();
    
    uint64_t ping_server_rate_in_us = DEFAULT_PING_RATE_US; 
    uint64_t reconnect_rate = MIN_RECONNECT_RATE_US;
    

    if (validateField(jsonConfig,"ping_server_rate_in_ms", Json_de::value_t::number_unsigned))
    {
        ping_server_rate_in_us = (uint64_t) jsonConfig["ping_server_rate_in_ms"].get<int>()  * 1000l;
    }

    if (validateField(jsonConfig,"max_allowed_ping_delay_in_ms", Json_de::value_t::number_unsigned))
    {
        reconnect_rate = jsonConfig["max_allowed_ping_delay_in_ms"].get<int>() * 1000l;
    }

    reconnect_rate = (reconnect_rate < MIN_RECONNECT_RATE_US)?MIN_RECONNECT_RATE_US:reconnect_rate;
  
    m_lasttime_access = get_time_usec();
    while (!m_exit)
    {
        const uint64_t diff =  get_time_usec() - m_lasttime_access;
        
        if (m_status == SOCKET_STATUS_REGISTERED)
        {
            off_count = 0;
            #ifdef DDEBUG
                std::cout << "you are ok" << std::endl;
            #endif
            API_pingServer();
        }
        else
        {
            std::cout  << _LOG_CONSOLE_BOLD_TEXT <<  "you are " << _ERROR_CONSOLE_BOLD_TEXT_  " OFFLINE " << _INFO_CONSOLE_TEXT << diff << _LOG_CONSOLE_BOLD_TEXT << " us" << _NORMAL_CONSOLE_TEXT_  << std::endl;
        }
#ifdef DEBUG
        if (off_count > 500) abort();
#else
        if (off_count > 5) abort();
#endif
        if (diff > reconnect_rate)
        {          
            off_count++;
            m_lasttime_access = get_time_usec();
            std::cout << "BOFT:" << diff << std::endl;
            if (_cwsa_session) 
            {
                _cwsa_session.get()->shutdown();
                onSocketError();    
            }
        }
        std::this_thread::sleep_for(std::chrono::microseconds(ping_server_rate_in_us));
    }

	return ;
}

/**
 * @brief Entry function for Connection.
 * 
 */
void de::andruav_servers::CAndruavCommServer::start ()
{
    
    if (m_exit) return ;

    if (!m_watch_dog)
    {
        m_watch_dog = std::make_unique<std::thread>([&](){ startWatchDogThread(); });
    }
    connect(); 
    
}

/**
 * @brief Main function that connects to Andruav Authentication
 * 
 */
void de::andruav_servers::CAndruavCommServer::connect ()
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
        {   // this is to protect rapid calls to this function without waiting for server to response.
            return ;
        }

        m_next_connect_time = now_time + MIN_RECONNECT_RATE_US; // retry after 10 sec.

        de::andruav_servers::CAndruavAuthenticator& andruav_auth = de::andruav_servers::CAndruavAuthenticator::getInstance();
        
        m_status = SOCKET_STATUS_CONNECTING;
        if (!andruav_auth.doAuthentication() || !andruav_auth.isAuthenticationOK())   
        {
            m_status = SOCKET_STATUS_ERROR;
            PLOG(plog::error) << "Communicator Server Connection Status: SOCKET_STATUS_ERROR"; 
            de::comm::CUavosModulesManager::getInstance().handleOnAndruavServerConnection (m_status);
            return ;
        }
    

        std::string serial;
        if (helpers::CUtil_Rpi::getInstance().get_cpu_serial(serial)!= false)
        {
            std::cout << "Unique Key :" << serial << std::endl;
        }
        serial.append(get_linux_machine_id());

        de::ANDRUAV_UNIT_INFO&  unit_info = de::CAndruavUnitMe::getInstance().getUnitInfo();
    
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
 * @brief Disconnect websocket for a time duration
 * 
 * @param on_off 
 * @param duration in seconds
 */
void de::andruav_servers::CAndruavCommServer::turnOnOff(const bool on_off, const uint32_t duration_seconds)
{
    m_on_off_delay  = duration_seconds;
    if (on_off)
    {
        std::cout << _INFO_CONSOLE_BOLD_TEXT << "WS Module:" << _LOG_CONSOLE_TEXT << " Set Communication Line " << _SUCCESS_CONSOLE_BOLD_TEXT_ <<  " Switched Online" << _LOG_CONSOLE_TEXT <<  " duration (sec): "  << _SUCCESS_CONSOLE_BOLD_TEXT_ << std::to_string(duration_seconds) << _NORMAL_CONSOLE_TEXT_ << std::endl;

        g = std::thread {[&](){ 
            try
            {
                m_exit = false;
                if (m_on_off_delay!=0)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(m_on_off_delay));
                    std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WS Module:" << _LOG_CONSOLE_TEXT << "Set Communication Line " << _ERROR_CONSOLE_BOLD_TEXT_ <<  " Switched Offline" <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
                    uninit(true);
                }
                
                g.detach();
            }
            catch (...)
            {
               std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WS Module:" << _LOG_CONSOLE_TEXT << "Set Communication Line " << _ERROR_CONSOLE_BOLD_TEXT_ <<  " EXCEPTION" <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
            }
        }};
    
    }
    else
    {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WS Module:" << _LOG_CONSOLE_TEXT << "Set Communication Line " << _ERROR_CONSOLE_BOLD_TEXT_ <<  " Switched Offline" << _LOG_CONSOLE_TEXT <<  " duration (sec): " << _SUCCESS_CONSOLE_BOLD_TEXT_ << std::to_string(duration_seconds) << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
        de::andruav_servers::CAndruavFacade::getInstance().API_sendCommunicationLineStatus(std::string(), false);
    
        g = std::thread {[&](){ 
            try
            {
                std::this_thread::sleep_for(std::chrono::seconds(1)); // wait for message to be sent.
                        
                uninit(true);
                    
                if (m_on_off_delay!=0)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(m_on_off_delay));
                    std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WS Module:" << _LOG_CONSOLE_TEXT << "Set Communication Line " << _ERROR_CONSOLE_BOLD_TEXT_ <<  " Restart" <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
                        
                    // re-enable.
                    m_exit = false;
                }
                    
                g.detach();
            }
            catch (...)
            {
                std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WS Module:" << _LOG_CONSOLE_TEXT << "Set Communication Line " << _ERROR_CONSOLE_BOLD_TEXT_ <<  " EXCEPTION" <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
            }
        }};
    }
                    
                
}


/**
 * @brief Connects to Andruav Communication Server. 
 * This function uses information received form AuthServer.
 * 
 * @param server_ip 
 * @param server_port 
 * @param key 
 * @param party_id 
 */
void de::andruav_servers::CAndruavCommServer::connectToCommServer (const std::string& server_ip, const std::string &server_port, const std::string& key, const std::string& party_id)
{
    try
    {
        
        m_host = std::string(server_ip);
        m_port = std::string(server_port);
        m_party_id = std::string(party_id);

        m_url_param = "/?f=" + key + "&s=" + m_party_id;
        
        // Launch Synchronous Socket
        if (_cwsa_session)
        {
            _cwsa_session.get()->shutdown();
        }
        
        _cwsa_session = _cwsa_proxy.run(m_host.c_str(), m_port.c_str(), m_url_param.c_str(), *this);
        
        // To delay the auto retry
        m_lasttime_access = get_time_usec();

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


void de::andruav_servers::CAndruavCommServer::onSocketError()
{
    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: onSocketError " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    if (m_exit)
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

    de::comm::CUavosModulesManager::getInstance().handleOnAndruavServerConnection (m_status);

}

/**
 * @brief 
 * 
 * @param message first part until byte of value'0' should be XML header.
 * @param datalength 
 */
void de::andruav_servers::CAndruavCommServer::onBinaryMessageRecieved (const char * message, const std::size_t datalength)
{
    m_lasttime_access = get_time_usec();

    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: onBinaryMessageRecieved " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    Json_de jMsg;
    jMsg = Json_de::parse(message);
    if (!validateField(jMsg, INTERMODULE_ROUTING_TYPE, Json_de::value_t::string))
    {
        // bad message format
        return ;
    }

    if (!validateField(jMsg, ANDRUAV_PROTOCOL_MESSAGE_TYPE, Json_de::value_t::number_unsigned))
    {
        // bad message format
        return ;
    }


    if (jMsg[INTERMODULE_ROUTING_TYPE].get<std::string>().compare(CMD_TYPE_SYSTEM_MSG)==0)
    {   // System Message
        
    }
    else
    {
        if (!validateField(jMsg, ANDRUAV_PROTOCOL_SENDER, Json_de::value_t::string))
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

        de::comm::CUavosModulesManager::getInstance().processIncommingServerMessage(sender, command_type,  message, datalength, std::string());
    }
    
}
            

/**
 * @brief text message recieved from ANdruavServerComm.
 * 
 * @param jsonMessage string message in JSON format.
 */
void de::andruav_servers::CAndruavCommServer::onTextMessageRecieved(const std::string& jsonMessage)
{
    m_lasttime_access = get_time_usec();

    #ifdef DDEBUG || DDEBUG_PARSER
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: onMessageRecieved " << jsonMessage << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    Json_de jMsg;
    jMsg = Json_de::parse(jsonMessage);
    if (!validateField(jMsg, INTERMODULE_ROUTING_TYPE, Json_de::value_t::string))
    {
        // bad message format
        return ;
    }

    if (!validateField(jMsg, ANDRUAV_PROTOCOL_MESSAGE_TYPE, Json_de::value_t::number_unsigned))
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
        m_lasttime_access = get_time_usec();
        de::comm::CUavosModulesManager::getInstance().handleOnAndruavServerConnection (m_status);
    }


    if (jMsg[INTERMODULE_ROUTING_TYPE].get<std::string>().compare(CMD_TYPE_SYSTEM_MSG)==0)
    {   // Handle Communication Server SYSTEM messages... this is communication establish message and not _SYS_ messages.
        const int command_type = jMsg[ANDRUAV_PROTOCOL_MESSAGE_TYPE].get<int>();
        switch (command_type)
        {
            // Server replied when connection  has been established.
            case TYPE_AndruavSystem_ConnectedCommServer:
            {   
                // example onMessageRecieved {"ty":"s","mt":9007,"ms":{"s":"OK:connected:tcp:192.168.1.144:37196"}}
                Json_de message_cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
                if (message_cmd["s"].get<std::string>().find("OK")==0)
                {
                    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "Communication Server Connected: Success "  << _NORMAL_CONSOLE_TEXT_ << std::endl;
                    PLOG(plog::info) << "Communication Server Connected: Success ";
                    
                    m_status = SOCKET_STATUS_REGISTERED;
                    m_lasttime_access = get_time_usec();
                    de::andruav_servers::CAndruavFacade::getInstance().API_requestID(std::string());
                }
                else
                {
                    std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Communication Server Connected: Failed "  << _NORMAL_CONSOLE_TEXT_ << std::endl;
                    PLOG(plog::error) << "Communication Server Connected: Failed "; 

                    m_status = SOCKET_STATUS_ERROR;
                }
                de::comm::CUavosModulesManager::getInstance().handleOnAndruavServerConnection (m_status);
            }
            break;

            default:
                
                break;
        }
    }
    else
    {
        if (!validateField(jMsg, ANDRUAV_PROTOCOL_SENDER, Json_de::value_t::string))
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

        de::comm::CUavosModulesManager::getInstance().processIncommingServerMessage(sender, command_type,  jsonMessage.c_str(), jsonMessage.length(), std::string());
    }
}


void de::andruav_servers::CAndruavCommServer::parseCommand (const std::string& sender_party_id, const int& command_type, const Json_de& jsonMessage)
{
    #ifdef DDEBUG_PARSER
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: parseCommand " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    // retreive unit or create a new one
    de::CAndruavUnit* unit = m_andruav_units.getUnitByName(sender_party_id);
    ANDRUAV_UNIT_INFO& unit_info = unit->getUnitInfo();

    uint32_t permission = 0;
    if (validateField(jsonMessage, ANDRUAV_PROTOCOL_MESSAGE_PERMISSION, Json_de::value_t::number_unsigned))
    {
        permission =  jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_PERMISSION].get<int>();
    }
    UNUSED (permission);

    bool is_system = false;
    if ((validateField(jsonMessage, ANDRUAV_PROTOCOL_SENDER, Json_de::value_t::string)) && (jsonMessage[ANDRUAV_PROTOCOL_SENDER].get<std::string>().compare(SPECIAL_NAME_SYS_NAME)==0))
    {   // permission is not needed if this command sender is the communication server not a remote GCS or Unit.
        is_system = true;
    }
    

    // get message command details
    const Json_de& msg_cmd = jsonMessage.contains(ANDRUAV_PROTOCOL_MESSAGE_CMD)?jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD]:Json_de();
    
    // if unit is new then ask for details.
    if ((command_type!=TYPE_AndruavMessage_ID) && (unit_info.is_new == true))  
    {
        de::andruav_servers::CAndruavFacade::getInstance().API_requestID (sender_party_id);    
        
        /*
            DONOT add return here unless system requires more security.
            You cannot receive messages except via Communication Server from units that are logged into the system so it should be secure.
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
            const Json_de command = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD];
            unit_info.vehicle_type = command["VT"].get<int>();
            unit_info.is_gcs = command["GS"].get<bool>();
            
            
            unit_info.party_id = sender_party_id;
            
            if (!validateField(command,"UD", Json_de::value_t::string)) return ;
            if (!validateField(command,"DS", Json_de::value_t::string)) return ;
            
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
            
            if (command.contains("n") == true) unit_info.swarm_follower_formation = command["n"].get<int>();
            if (command.contains("o") == true) unit_info.swarm_leader_formation = command["o"].get<int>();
            if (command.contains("q") == true) unit_info.swarm_leader_I_am_following = command["q"].get<std::string>();
            
            
            // if is for backward compatibility
            if (command.contains("T") == true) unit_info.unit_sync_time = command["T"];
            unit_info.last_access_time = get_time_usec();
            
            // std::string res = unit_info.is_new?"New":"OLD" ;
            // std::cout << _INFO_CONSOLE_TEXT << "TYPE_AndruavMessage_ID: " << unit_info.is_new << _SUCCESS_CONSOLE_TEXT_ <<  msg_cmd.dump() << _NORMAL_CONSOLE_TEXT_ << std::endl;

            unit_info.is_new = false;
            
        }
        break;

        case TYPE_AndruavMessage_Unit_Name:
        {
            /*
                Set Unit Name
                TYPE_AndruavMessage_Unit_Name
                UN:string: unit name
                DS:string: unit description
                PR: true/false [optional]
            */

            const Json_de command = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD];
            
            if (!validateField(command,"UN", Json_de::value_t::string)) return ;
            if (!validateField(command,"DS", Json_de::value_t::string)) return ;

            de::CAndruavUnitMe& m_andruavMe = de::CAndruavUnitMe::getInstance();
            de::ANDRUAV_UNIT_INFO&  unit_info = m_andruavMe.getUnitInfo();

            unit_info.unit_name   = command["UN"].get<std::string>();
            unit_info.description = command["DS"].get<std::string>();

            de::CLocalConfigFile& cLocalConfigFile = de::CLocalConfigFile::getInstance();
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
                  Change will take effective after reboot or server disconnection.
                  Also if server is asking for fixed UDP port then communication server will allocate the port for the old one.
                */
                //unit_info.party_id = party_id;   << do not uncomment.
            }
            de::andruav_servers::CAndruavFacade::getInstance().API_sendID(sender_party_id);
   
        }
        break;

        case TYPE_AndruavMessage_Communication_Line_Set:
        {
            /**
             * @brief used to set communication channels on/off
             * current fields are:
             * [p2p]: for turning p2p on/off or leave as is.
             * [ws]: for turning communication server websocket on/off or leave as is.
             * 
            */

            const Json_de command = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD];
            
            if (validateField(command,"ws", Json_de::value_t::boolean))
            {
                bool ws_on_off = command["ws"].get<bool>();
                uint32_t ws_duration = 0;
                if (validateField(command,"wsd", Json_de::value_t::number_unsigned))
                {
                    ws_duration = command["wsd"].get<int>();  
                }
                turnOnOff(ws_on_off, ws_duration);
            }

            // Message can be handled by other modules.
        }
        break;
    }

}


void de::andruav_servers::CAndruavCommServer::parseRemoteExecuteCommand (const std::string& sender_party_id, const Json_de& jsonMessage)
{
    #ifdef DDEBUG_PARSER
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: parseRemoteExecuteCommand " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    const Json_de& msg_cmd = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD];
    
    uint32_t permission = 0;
    if (validateField(jsonMessage, ANDRUAV_PROTOCOL_MESSAGE_PERMISSION, Json_de::value_t::number_unsigned))
    {
        permission =  jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_PERMISSION].get<int>();
    }
    UNUSED (permission);

    bool is_system = false;
    if ((validateField(jsonMessage, ANDRUAV_PROTOCOL_SENDER, Json_de::value_t::string)) && (jsonMessage[ANDRUAV_PROTOCOL_SENDER].get<std::string>().compare(SPECIAL_NAME_SYS_NAME)==0))
    {   // permission is not needed if this command sender is the communication server not a remote GCS or Unit.
        is_system = true;
    }


    if (!msg_cmd.contains("C")) return ;

    int remote_execute_command = msg_cmd["C"];

    de::CAndruavUnit* unit = m_andruav_units.getUnitByName(sender_party_id);
    ANDRUAV_UNIT_INFO& unit_info = unit->getUnitInfo();
    
    if ((unit_info.is_new == true) &&(remote_execute_command!=TYPE_AndruavMessage_ID)) 
    {
        // the sender is a new unit so we ask for identification. ... 
        
        de::andruav_servers::CAndruavFacade::getInstance().API_requestID (sender_party_id);    // ask for identification in return.      
        // !IF YOU ADD RETURN HERE THEN API_requestID will MAKE PING PONG

        /*
            !DONOT add return here unless system requires more security.

            You cannot receive messages except via Communication Server from units that are logged into the system so it should be secure.
            ?If you enable the return the following issues may happen:
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
            de::andruav_servers::CAndruavFacade::getInstance().API_sendID(sender_party_id);
        }
        break;

        case TYPE_AndruavMessage_CameraList:
        {
            de::andruav_servers::CAndruavFacade::getInstance().API_sendCameraList (true, sender_party_id);
        }
        break;

        case RemoteCommand_STREAMVIDEO:
        {
            if (!validateField(msg_cmd, "Act", Json_de::value_t::boolean))
            {
                // bad message format
                return ;
            }
            if (msg_cmd["Act"].get<bool>()==true)
            {
                de::andruav_servers::CAndruavFacade::getInstance().API_sendCameraList (true, sender_party_id);
            }
        }
		break;

        case RemoteCommand_TELEMETRYCTRL:
        {
            if (!validateField(msg_cmd, "Act", Json_de::value_t::number_unsigned))
            {
                // bad message format
                return ;
            }
            const int request_type = msg_cmd["Act"].get<int>();
            if (request_type != CONST_TELEMETRY_ADJUST_RATE) return ;
            
            int streaming_level = -1;
            if (!validateField(msg_cmd, "LVL", Json_de::value_t::number_unsigned))
            {
                // bad message format
                return ;
            }
            streaming_level = msg_cmd["LVL"].get<int>();
            de::STATUS& status = de::STATUS::getInstance();
            status.streaming_level(streaming_level);
        }
        break;

    }
}
            

void de::andruav_servers::CAndruavCommServer::uninit(const bool exit_mode)
{
    
    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: uninit " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    PLOG(plog::info) << "uninit initiated."; 
        
    m_exit = exit_mode;
    
    if (_cwsa_session)
    {
        _cwsa_session.get()->shutdown();
        _cwsa_session.reset();
        _cwsa_session.release();
        _cwsa_session = nullptr;
    }
    
    struct timespec ts;
           int s;

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        exit(0);
    }
    ts.tv_sec += 10;

    m_watch_dog->join(); // Wait for the thread to exit
    m_watch_dog.release();
    #ifdef DEBUG
        std::cout << __PRETTY_FUNCTION__ <<  _LOG_CONSOLE_TEXT << "DEBUG: m_watch_dog 1" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        exit(0);
    }
    ts.tv_sec += 10;

    s = pthread_timedjoin_np(m_watch_dog2, NULL, &ts);
    if (s != 0) {
        //exit(0);
    }
    #ifdef DEBUG
        std::cout << __PRETTY_FUNCTION__ <<  _LOG_CONSOLE_TEXT << "DEBUG: m_watch_dog 2" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    m_status = SOCKET_STATUS_FREASH;
	
    PLOG(plog::info) << "uninit finished."; 
    
    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: uninit OUT " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
}


void de::andruav_servers::CAndruavCommServer::API_pingServer()
{
    Json_de message =  { 
        {"t", get_time_usec()}
    };


    API_sendSystemMessage(TYPE_AndruavSystem_Ping, message);
}

void de::andruav_servers::CAndruavCommServer::API_sendSystemMessage(const int command_type, const Json_de& msg) const 
{
    if (m_status == SOCKET_STATUS_REGISTERED)  
    {
        if (!_cwsa_session) return;

        Json_de json_msg  = this->generateJSONSystemMessage (command_type, msg);
        _cwsa_session.get()->writeText(json_msg.dump());
    } 
}
            


/// @details Sends Andruav Command to Communication Server
/// *_GCS_: broadcast to GCS.
/// *_AGN_: broadcast to vehicles only.
/// *_GD_: broadcast to all..
/// *null: means send to all units if sender is GCS, and if sender is drone means send to all GCS.
/// @param target_name party_id of a target or can be null or _GD_, _AGN_, _GCS_
/// @param command_type 
/// @param msg 
void de::andruav_servers::CAndruavCommServer::API_sendCMD (const std::string& target_name, const int command_type, const Json_de& msg)
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
        if (!_cwsa_session)  return ;

        Json_de json_msg  = this->generateJSONMessage (message_routing, m_party_id, target_name, command_type, msg);
        _cwsa_session.get()->writeText(json_msg.dump());
    } 
}

std::string de::andruav_servers::CAndruavCommServer::API_sendCMDDummy (const std::string& target_name, const int command_type, const Json_de& msg)
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
        Json_de json_msg  = this->generateJSONMessage (message_routing, m_party_id, target_name, command_type, msg);
        return json_msg.dump();
    } 

    return "";
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
void de::andruav_servers::CAndruavCommServer::API_sendBinaryCMD (const std::string& target_party_id, const int command_type, const char * bmsg, const uint64_t bmsg_length, const Json_de& message_cmd)
{
    static std::mutex g_i_mutex; 

    const std::lock_guard<std::mutex> lock(g_i_mutex);
    
    #ifdef DDEBUG_MSG        
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
        
        Json_de json  = this->generateJSONMessage (message_routing, m_party_id, target_party_id, command_type, message_cmd);
        std::string json_msg = json.dump();
        char * msg_ptr = new char[json_msg.length() + 1 + bmsg_length];
        strcpy(msg_ptr,json_msg.c_str());
        msg_ptr[json_msg.length()] = 0;
        memcpy(&msg_ptr[json_msg.length()+1], bmsg, bmsg_length);
        if (_cwsa_session) 
        {
            _cwsa_session.get()->writeBinary(msg_ptr, json_msg.length() + 1 + bmsg_length);
        }

        delete[] msg_ptr;
    } 
}


/**
* @brief 
*  Send  message to communication server by calling API_sendCMD or API_sendBinaryCMD
* 
* @param full_message : full message received from module including the header JSON part before the binary.
* @param full_message_length 
* @param is_system 
* @param is_binary 
* @param target_id 
* @param msg_type : i.e. Message ID
* @param msg_cmd : Message ID Parameters... i.e. JSOM command of the message
*/
void de::andruav_servers::CAndruavCommServer::sendMessageToCommunicationServer (const char * full_message, const std::size_t full_message_length, const bool &is_system, const bool &is_binary, const std::string &target_id, const int msg_type, const Json_de &msg_cmd )
{
    if (is_binary)
    {
        // Search for char '0' and then the binary message is the bytes after it.
        // i.e. remove the text header of the ogirinal message.
        // the command part already exists in msg_cmd.
        const std::size_t first_string_length = strlen (full_message);
        std::size_t binary_length = full_message_length - first_string_length;
            
        API_sendBinaryCMD(target_id, msg_type, &full_message[first_string_length+1], binary_length-1, msg_cmd);
    }
    else if (is_system)
    {
        // Send message to Communication Server to be processed by Communication Server as this is a system message.
        API_sendSystemMessage(msg_type, msg_cmd);    
    }
    else 
    {
        // Send message to other parties via Communication Server.
        API_sendCMD(target_id, msg_type, msg_cmd);            
    }
}


/// @brief 
/// 
/// @param message_routing @link CMD_COMM_GROUP @endlink, @link CMD_COMM_INDIVIDUAL @endlink
/// @param sender_name 
/// @param target_party_id  single target except for the following
///  *_GD_* all GCS
///  *_AGN_* all agents
/// @param messageType 
/// @param message 
/// @return Json_de 
Json_de de::andruav_servers::CAndruavCommServer::generateJSONMessage (const std::string& message_routing, const std::string& sender_name, const std::string& target_party_id, const int messageType, const Json_de& message) const
{

    #ifdef DDEBUG_MSG        
    
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "generateJSONMessage " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    Json_de jMsg;
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

/// @brief Generate JSON message for a system message.
/// @param messageType 
/// @param message 
/// @return Json_de object of the message.
Json_de de::andruav_servers::CAndruavCommServer::generateJSONSystemMessage (const int messageType, const Json_de& message) const
{
    #ifdef DDEBUG_MSG        
    
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "generateJSONMessage " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    Json_de jMsg;
    jMsg[INTERMODULE_ROUTING_TYPE]      = CMD_COMM_SYSTEM;
    jMsg[ANDRUAV_PROTOCOL_MESSAGE_TYPE] = messageType;
    jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD]  = message;
    

    return jMsg;
}

