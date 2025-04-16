

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
#include "andruav_auth.hpp"
#include "andruav_unit.hpp"
#include "../de_broker/de_modules_manager.hpp"
#include "andruav_comm_server.hpp"
#include "andruav_facade.hpp"
#include "andruav_parser.hpp"


std::thread g1;
// Based on Below Model
// https://www.boost.org/doc/libs/develop/libs/beast/example/websocket/client/async-ssl/websocket_client_async_ssl.cpp

// ------------------------------------------------------------------------------
//  Pthread Starter Helper Functions
// ------------------------------------------------------------------------------


using namespace de::andruav_servers;


void CAndruavCommServer::startWatchDogThread()
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
void CAndruavCommServer::start ()
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
void CAndruavCommServer::connect ()
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

        CAndruavAuthenticator& andruav_auth = CAndruavAuthenticator::getInstance();
        
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
void CAndruavCommServer::turnOnOff(const bool on_off, const uint32_t duration_seconds)
{
    m_on_off_delay = duration_seconds;
    if (on_off)
    {
        std::cout << _INFO_CONSOLE_BOLD_TEXT << "WS Module:" << _LOG_CONSOLE_TEXT << " Set Communication Line " << _SUCCESS_CONSOLE_BOLD_TEXT_ <<  " Switched Online" << _LOG_CONSOLE_TEXT <<  " duration (sec): "  << _SUCCESS_CONSOLE_BOLD_TEXT_ << std::to_string(duration_seconds) << _NORMAL_CONSOLE_TEXT_ << std::endl;

        // Create and immediately detach the thread
        g1 = std::thread{[this]() { 
            try
            {
                // Switch online
                m_exit = false;
                if (m_on_off_delay != 0)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(m_on_off_delay));
                    // Switch offline again after delay
                    std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WS Module:" << _LOG_CONSOLE_TEXT << "Set Communication Line " << _ERROR_CONSOLE_BOLD_TEXT_ <<  " Switched Offline" <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
                    uninit(true);
                }
            }
            catch (...)
            {
               std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WS Module:" << _LOG_CONSOLE_TEXT << "Set Communication Line " << _ERROR_CONSOLE_BOLD_TEXT_ <<  " EXCEPTION" <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
            }
        }};
        g1.detach(); // Detach immediately after creation
    }
    else
    {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WS Module:" << _LOG_CONSOLE_TEXT << "Set Communication Line " << _ERROR_CONSOLE_BOLD_TEXT_ <<  " Switched Offline" << _LOG_CONSOLE_TEXT <<  " duration (sec): " << _SUCCESS_CONSOLE_BOLD_TEXT_ << std::to_string(duration_seconds) << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
        CAndruavFacade::getInstance().API_sendCommunicationLineStatus(std::string(), false);
    
        // Create and immediately detach the thread
        g1 = std::thread{[this]() { 
            try
            {
                std::this_thread::sleep_for(std::chrono::seconds(1)); // wait for message to be sent.
                        
                uninit(true);
                    
                if (m_on_off_delay != 0)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(m_on_off_delay));
                    std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WS Module:" << _LOG_CONSOLE_TEXT << "Set Communication Line " << _ERROR_CONSOLE_BOLD_TEXT_ <<  " Restart" <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
                        
                    // re-enable.
                    m_exit = false;
                }
            }
            catch (...)
            {
                std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WS Module:" << _LOG_CONSOLE_TEXT << "Set Communication Line " << _ERROR_CONSOLE_BOLD_TEXT_ <<  " EXCEPTION" <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
            }
        }};
        g1.detach(); // Detach immediately after creation
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
void CAndruavCommServer::connectToCommServer (const std::string& server_ip, const std::string &server_port, const std::string& key, const std::string& party_id)
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
        
        _cwsa_session = _cwsa_proxy.run1(m_host.c_str(), m_port.c_str(), m_url_param.c_str(), *this);
        
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
        if (_cwsa_session) {
            _cwsa_session.reset();
        }
        return ;
    }
}


void CAndruavCommServer::onSocketError()
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
void CAndruavCommServer::onBinaryMessageRecieved (const char * message, const std::size_t datalength)
{
    try
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
                    CAndruavParser::getInstance().parseRemoteExecuteCommand(sender, jMsg);
                }
                break;

                default:
                {
                    CAndruavParser::getInstance().parseCommand(sender, command_type, jMsg);
                }
                break;
            }

            de::comm::CUavosModulesManager::getInstance().processIncommingServerMessage(sender, command_type,  message, datalength, std::string());
        }
    }

    catch(const std::exception& e)
    {
        #ifdef DEBUG
            std::cerr <<__PRETTY_FUNCTION__ <<  e.what() << '\n';
        #endif
    }
    
}
            

/**
 * @brief text message recieved from ANdruavServerComm.
 * 
 * @param jsonMessage string message in JSON format.
 */
void CAndruavCommServer::onTextMessageRecieved(const std::string& jsonMessage)
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
                    CAndruavFacade::getInstance().API_requestID(std::string());
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
                CAndruavParser::getInstance().parseRemoteExecuteCommand(sender, jMsg);
            }
            break;

            default:
            {
                CAndruavParser::getInstance().parseCommand(sender, command_type, jMsg);
            }
            break;
        }

        de::comm::CUavosModulesManager::getInstance().processIncommingServerMessage(sender, command_type,  jsonMessage.c_str(), jsonMessage.length(), std::string());
    }
}

            

void CAndruavCommServer::uninit(const bool exit_mode)
{
    try
    {
    
    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: uninit " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    PLOG(plog::info) << "uninit initiated."; 
        
    m_exit = exit_mode;
    
    struct timespec ts;
    std::cout <<__PRETTY_FUNCTION__ << " line 1:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: uninit " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    
    try
    {
        if (_cwsa_session) {
            boost::asio::io_context io;
            boost::asio::steady_timer timer(io, std::chrono::seconds(5)); // 5-second timeout

            bool shutdown_completed = false;

            // Start the timer
            timer.async_wait([&](boost::system::error_code ec) {
                if (!ec) {
                    // Timeout occurred
                    if (!shutdown_completed) {
                        std::cerr << _ERROR_CONSOLE_BOLD_TEXT_ << "Shutdown timed out! Forcefully closing." << _NORMAL_CONSOLE_TEXT_ << std::endl;
                        _cwsa_session->close(); // Forcefully close
                    }
                }
            });

            // Perform the shutdown
            _cwsa_session->shutdown();
            shutdown_completed = true;
            timer.cancel(); // Cancel the timer if shutdown succeeds.

            _cwsa_session.reset();
        } 
    }   catch(const std::exception& e)
    {
        std::cerr << __PRETTY_FUNCTION__ << " line 1:" << __LINE__ << e.what() << '\n';
    }
    
    #ifdef DEBUG
    std::cout <<__PRETTY_FUNCTION__ << " line 2:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: uninit " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
            std::cout << __PRETTY_FUNCTION__ <<  _LOG_CONSOLE_TEXT << "DEBUG: EXIT" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            exit(0);
    }
    
    #ifdef DEBUG
    std::cout <<__PRETTY_FUNCTION__ << " line:3" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: uninit " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    ts.tv_sec += 10;
    if (m_watch_dog && m_watch_dog->joinable())
    {
        m_watch_dog->join(); // Wait for the thread to exit
    }
    
    if (m_watch_dog)
    {
        m_watch_dog.release();
    }
    #ifdef DEBUG
        std::cout << __PRETTY_FUNCTION__ <<  _LOG_CONSOLE_TEXT << "DEBUG: m_watch_dog 1" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        exit(0);
    }
    ts.tv_sec += 10;

    
    #ifdef DEBUG
        std::cout << __PRETTY_FUNCTION__ <<  _LOG_CONSOLE_TEXT << "DEBUG: m_watch_dog 2" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    m_status = SOCKET_STATUS_FRESH;
	
    PLOG(plog::info) << "uninit finished."; 
    
    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: uninit OUT " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
     }
    catch(const std::exception& e)
    {
        #ifdef DEBUG
            std::cerr << "Unint ERROR " << e.what() << '\n';
        #endif
    }
    
    
}


void CAndruavCommServer::API_pingServer()
{
    Json_de message =  { 
        {"t", get_time_usec()}
    };


    API_sendSystemMessage(TYPE_AndruavSystem_Ping, message);
}

void CAndruavCommServer::API_sendSystemMessage(const int command_type, const Json_de& msg) const 
{
    if (m_status != SOCKET_STATUS_REGISTERED) return ;
    if (!_cwsa_session)  return ;


    Json_de json_msg  = m_andruav_message.generateJSONSystemMessage (command_type, msg);
    _cwsa_session.get()->writeText(json_msg.dump());
    
}
            


/// @details Sends Andruav Command to Communication Server
/// *_GCS_: broadcast to GCS.
/// *_AGN_: broadcast to vehicles only.
/// *_GD_: broadcast to all..
/// *null: means send to all units if sender is GCS, and if sender is drone means send to all GCS.
/// @param target_name party_id of a target or can be null or _GD_, _AGN_, _GCS_
/// @param command_type 
/// @param msg 
void CAndruavCommServer::API_sendCMD (const std::string& target_name, const int command_type, const Json_de& msg)
{
    static std::mutex g_i_mutex; 

    const std::lock_guard<std::mutex> lock(g_i_mutex);
    
    if (m_status != SOCKET_STATUS_REGISTERED) return ;
    if (!_cwsa_session)  return ;

    std::string message_routing;
    if (target_name.empty() == false)
    {  // BUG HERE PLease ensure that it sends ind.
        message_routing = CMD_COMM_INDIVIDUAL;
    }
    else
    {
        message_routing = CMD_COMM_GROUP;
    }

    Json_de json_msg  = m_andruav_message.generateJSONMessage (message_routing, m_party_id, target_name, command_type, msg);
    _cwsa_session.get()->writeText(json_msg.dump());

}

std::string CAndruavCommServer::API_sendCMDDummy (const std::string& target_name, const int command_type, const Json_de& msg)
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
        Json_de json_msg  = m_andruav_message.generateJSONMessage (message_routing, m_party_id, target_name, command_type, msg);
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
void CAndruavCommServer::API_sendBinaryCMD (const std::string& target_party_id, const int command_type, const char * bmsg, const uint64_t bmsg_length, const Json_de& message_cmd)
{
    static std::mutex g_i_mutex; 

    const std::lock_guard<std::mutex> lock(g_i_mutex);
    
    #ifdef DDEBUG_MSG        
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "API_sendCMD " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    if (m_status != SOCKET_STATUS_REGISTERED) return ;
    if (!_cwsa_session)  return ;

    std::string message_routing;
    if (target_party_id.empty() == false)
    {
        message_routing = CMD_COMM_INDIVIDUAL;
    }
    else
    {
        message_routing = CMD_COMM_GROUP;
    }

    Json_de json  = m_andruav_message.generateJSONMessage (message_routing, m_party_id, target_party_id, command_type, message_cmd);
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
void CAndruavCommServer::sendMessageToCommunicationServer (const char * full_message, const std::size_t full_message_length, const bool &is_system, const bool &is_binary, const std::string &target_id, const int msg_type, const Json_de &msg_cmd )
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


void CAndruavCommServer::switchOnline()
{
    m_exit = false;
}

void CAndruavCommServer::switchOffline()
{
    uninit(true);
}