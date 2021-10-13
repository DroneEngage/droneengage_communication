

#include <cstdlib>
#include <string>
#include <iostream>


#define BOOST_BEAST_ALLOW_DEPRECATED



#include "../helpers/colors.hpp"
#include "../helpers/helpers.hpp"
#include "../helpers/util_rpi.hpp"

#include "../messages.hpp"
#include "../configFile.hpp"
#include "andruav_auth.hpp"
#include "../uavos/uavos_modules_manager.hpp"
#include "andruav_comm_server.hpp"

// Based on Below Model
// https://www.boost.org/doc/libs/develop/libs/beast/example/websocket/client/async-ssl/websocket_client_async_ssl.cpp

// ------------------------------------------------------------------------------
//  Pthread Starter Helper Functions
// ------------------------------------------------------------------------------




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
                    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: andruav_server.shouldExit() == true" << _NORMAL_CONSOLE_TEXT_ << std::endl;
                #endif

                return NULL;
            }
	        usleep(100000); 
        }

        
    }

	return NULL;
}


void uavos::andruav_servers::CAndruavCommServer::start ()
{
    const int result = pthread_create( &m_watch_dog, NULL, &uavos::andruav_servers::startWatchDogThread, this );

    if ( result ) throw result;

}

void uavos::andruav_servers::CAndruavCommServer::connect ()
{
    try
    {

        if ((m_status == SOCKET_STATUS_REGISTERED) || (m_status == SOCKET_STATUS_CONNECTING))
        {
            
            return ;
        }

        const uint64_t now_time = get_time_usec();
        
        if (m_next_connect_time > now_time)
        {
            return ;
        }

        m_next_connect_time = now_time + 10000000l; // retry after 10 sec.

        uavos::andruav_servers::CAndruavAuthenticator& andruav_auth = uavos::andruav_servers::CAndruavAuthenticator::getInstance();
        uavos::CConfigFile& cConfigFile = uavos::CConfigFile::getInstance();
        const Json& jsonConfig = cConfigFile.GetConfigJSON();

        
        m_status = SOCKET_STATUS_CONNECTING;
        
        if (!andruav_auth.doAuthentication())   
        {
            m_status = SOCKET_STATUS_ERROR;
            return ;
        }
    

        std::string serial;
        if (helpers::CUtil_Rpi::getInstance().get_cpu_serial(serial)!=-1)
        {
            std::cout << "Unique Key :" << serial << std::endl;
        }
        serial.append(get_linux_machine_id());

        if (!andruav_auth.doValidateHardware(serial, AUTH_HARDWARE_TYPE_CPU))   
        {
            //m_status = SOCKET_STATUS_ERROR;
            //return ;
        }
        
        connectToCommServer(andruav_auth.m_comm_server_ip, std::to_string(andruav_auth.m_comm_server_port), andruav_auth.m_comm_server_key, jsonConfig["partyID"].get<std::string>() );

    }

    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return ;
    }
}

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
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: onSocketError Socket is Closed" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif
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
    if (m_exit== true)
    {
        m_status =  SOCKET_STATUS_DISCONNECTED;  
    }
    else
    {
        m_status = SOCKET_STATUS_ERROR;

        // TODO Send Internal Message to Modules telling them we are no longer connected.
    }

}

/**
 * @brief 
 * 
 * @param message first part until byte of value'0' should be XML header.
 * @param datalength 
 */
void uavos::andruav_servers::CAndruavCommServer::onBinaryMessageRecieved (const char * message, const std::size_t datalength)
{
    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: onBinaryMessageRecieved " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    Json jMsg;
    jMsg = Json::parse(message);
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
            case TYPE_AndruavResala_RemoteExecute:
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

        uavos::CUavosModulesManager& module_manager = uavos::CUavosModulesManager::getInstance();
        module_manager.processIncommingServerMessage(sender, command_type,  message, datalength);
    }
    
}
            

/**
 * @brief text message recieved from ANdruavServerComm.
 * 
 * @param jsonMessage string message in JSON format.
 */
void uavos::andruav_servers::CAndruavCommServer::onTextMessageRecieved(const std::string& jsonMessage)
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
                    //_cwssession.get()->writeText("OK");
                    API_requestID(std::string(""));
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
            case TYPE_AndruavResala_RemoteExecute:
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

        uavos::CUavosModulesManager& module_manager = uavos::CUavosModulesManager::getInstance();
        module_manager.processIncommingServerMessage(sender, command_type,  jsonMessage.c_str(), jsonMessage.length());
    }
}


void uavos::andruav_servers::CAndruavCommServer::parseCommand (const std::string& sender_party_id, const int& command_type, const Json& jsonMessage)
{
    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: parseCommand " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    uavos::CAndruavUnit* unit = m_andruav_units.getUnitByName(sender_party_id);
    const Json& msg_cmd = jsonMessage.contains(ANDRUAV_PROTOCOL_MESSAGE_CMD)?jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD]:Json();
    
    switch (command_type)
    {
        case TYPE_AndruavResala_ID:
        {
            /*
                TYPE_AndruavResala_ID
                GS:bool: is gcs
                UD:string: unit name
                DS:string: description
                VT:int: vehicle type

                FI:bool: useFCBIMU (optional default:false)
                
            */
            Json command = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD];
            ANDRUAV_UNIT_INFO& unit_info = unit->getUnitInfo();
            unit_info.vehicle_type = command["VT"].get<int>();
            unit_info.is_gcs = command["GS"].get<bool>();
            
            
            unit_info.party_id = sender_party_id;
            unit_info.unit_name = command["UD"].get<std::string>();
            unit_info.description = command["DS"].get<std::string>();
            
            if (command.contains("VR") == true) unit_info.is_video_recording = command["VR"].get<int>();
            if (command.contains("FI") == true) unit_info.use_fcb = command["FI"].get<bool>();
            if (command.contains("SD") == true) unit_info.is_shutdown = command["SD"].get<bool>();
            if (command.contains("GM") == true) unit_info.gps_mode = command["GM"].get<int>();
            if (command.contains("AR") == true) unit_info.is_armed = command["AR"].get<bool>();
            if (command.contains("FL") == true) unit_info.is_flying = command["FL"].get<bool>();
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
            unit_info.is_new = false;
        }
        break;
    }

}


void uavos::andruav_servers::CAndruavCommServer::parseRemoteExecuteCommand (const std::string& sender_party_id, const Json& jsonMessage)
{
    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: parseRemoteExecuteCommand " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    const Json& msg_cmd = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD];
    int remote_execute_command = msg_cmd["C"];

    uavos::CAndruavUnit* unit = m_andruav_units.getUnitByName(sender_party_id);
    ANDRUAV_UNIT_INFO& unit_info = unit->getUnitInfo();
    
    switch (remote_execute_command)
    {
        case TYPE_AndruavResala_ID:
        {
            this->API_sendID(sender_party_id);
    
            if (unit_info.is_new == true)  API_requestID (sender_party_id);    // ask for identification in return.  
    
        }
        break;

        case TYPE_AndruavResala_CameraList:
        {
            this->API_sendCameraList (true, sender_party_id);
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
                this->API_sendCameraList (true, sender_party_id);
            }
        }
		break;
					
    }
}
            

void uavos::andruav_servers::CAndruavCommServer::uninit()
{
    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: uninit " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    m_exit = true;
    
    _cwssession.get()->close();
    
    // wait for exit
	pthread_join(m_watch_dog ,NULL);
	

    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: uninit OUT " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
}


void uavos::andruav_servers::CAndruavCommServer::API_requestID (const std::string& target_party_id)
{
    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: API_requestID " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    Json jMsg = {{"C", TYPE_AndruavResala_ID}};
    
    API_sendCMD (target_party_id, TYPE_AndruavResala_RemoteExecute, jMsg.dump());
}


void uavos::andruav_servers::CAndruavCommServer::API_sendCameraList(const bool reply, const std::string& target_name)
{
    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: API_requestID " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    uavos::CUavosModulesManager& module_manager = uavos::CUavosModulesManager::getInstance();
    
    Json camera_list = module_manager.getCameraList();

    const Json jMsg = 
    {
        {"R", reply},
        {"T", camera_list}
    };
        
    API_sendCMD (target_name, TYPE_AndruavResala_CameraList, jMsg.dump());
}

void uavos::andruav_servers::CAndruavCommServer::API_sendID (const std::string& target_name)
{
    uavos::CConfigFile& cConfigFile = uavos::CConfigFile::getInstance();
    const Json& jsonConfig = cConfigFile.GetConfigJSON();


    uavos::CAndruavUnitMe& m_andruavMe = uavos::CAndruavUnitMe::getInstance();
    uavos::ANDRUAV_UNIT_INFO&  unit_info = m_andruavMe.getUnitInfo();
   

    Json jMsg = 
    {   
        {"VT", unit_info.vehicle_type},           // vehicle type
        {"GS", unit_info.is_gcs},                 // is gcs
        {"VR", unit_info.is_video_recording},     // is video recording
        {"B",  unit_info.is_gcs_blocked},         // is gcs blocked
        {"FM", unit_info.flying_mode},            // Flying mode
        {"GM", unit_info.gps_mode},               // gps mode
        {"TP", unit_info.telemetry_protocol},     // m_telemetry_protocol
        {"C",  unit_info.manual_TX_blocked_mode}, // Remote Control RX Mode
        {"UD", jsonConfig["unitID"]},             // unit Name
        {"DS", jsonConfig["unitDescription"]},    // unit Description
        {"p",  unit_info.permission}              // permissions
    };
 
    if (unit_info.is_tracking_mode)
    {
        jMsg["b"] = unit_info.is_tracking_mode;
    }
    if (unit_info.use_fcb)
    {
        jMsg["FI"] = unit_info.use_fcb;
    }
    if (unit_info.is_flying)
    {
        jMsg["FL"] = unit_info.is_flying;   // is flying or sinking
    }
    if (unit_info.is_armed)
    {
        jMsg["AR"] = unit_info.is_armed;    // is armed
    }
    if (unit_info.is_shutdown)
    {
        jMsg["SD"] = unit_info.is_shutdown;    // is armed
    }
    if (unit_info.is_flashing)
    {
        jMsg["x"] = unit_info.is_flashing;    // is flashing
    }
    if (unit_info.is_whisling)
    {
        jMsg["y"] = unit_info.is_whisling;    // is whisling
    }
    if (unit_info.swarm_leader_formation)
    {
        jMsg["o"] = unit_info.swarm_leader_formation;    
    }
    if (!unit_info.swarm_leader_I_am_following.empty())
    {
        jMsg["q"] = unit_info.swarm_leader_I_am_following;    
    }
    
    API_sendCMD (target_name, TYPE_AndruavResala_ID, jMsg.dump());
}


/**
 * @details Sends Andruav Command to Andruav Server
 *  *_GCS_: broadcast to GCS.
 *  *_AGN_: broadcast to vehicles only.
 *  *_GD_: broadcast to all..
 *  *null: means send to all units if sender is GCS, and if sender is drone means send to all GCS.
 * @param target_name party_id of a target or can be null or _GD_, _AGN_, _GCS_
 * @param command_type 
 * @param msg 
 */
void uavos::andruav_servers::CAndruavCommServer::API_sendCMD (const std::string& target_name, const int command_type, const std::string& msg)
{
    static std::mutex g_i_mutex; 

    const std::lock_guard<std::mutex> lock(g_i_mutex);
    
    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "API_sendCMD " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    std::string message_routing;
    if (target_name.empty() == false)
    {
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
        // std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "API_sendCMD " << jmsg.dump() << _NORMAL_CONSOLE_TEXT_ << std::endl;
        // #endif
    } 
}



/**
 * @details Sends Andruav Command to Andruav Server
 *  *_GCS_: broadcast to GCS.
 *  *_AGN_: broadcast to vehicles only.
 *  *_GD_: broadcast to all..
 *  *null: means send to all units if sender is GCS, and if sender is drone means send to all GCS.
 * @param target_party_id party_id of a target or can be null or _GD_, _AGN_, _GCS_
 * @param command_type 
 * @param bmsg 
 * @param bmsg_length
 */
void uavos::andruav_servers::CAndruavCommServer::API_sendBinaryCMD (const std::string& target_party_id, const int command_type, const char * bmsg, const int bmsg_length)
{
    static std::mutex g_i_mutex; 

    const std::lock_guard<std::mutex> lock(g_i_mutex);
    
    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "API_sendCMD " << _NORMAL_CONSOLE_TEXT_ << std::endl;
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
        Json json  = this->generateJSONMessage (message_routing, m_party_id, target_party_id, command_type, std::string(""));
        std::string json_msg = json.dump();
        char * msg_ptr = new char[json_msg.length() + 1 + bmsg_length];
        std::unique_ptr<char []> msg = std::unique_ptr<char []> (msg_ptr);
        strcpy(msg_ptr,json_msg.c_str());
        msg_ptr[json_msg.length()] = 0;
        memcpy(&msg[json_msg.length()+1], bmsg, bmsg_length);


        _cwssession.get()->writeBinary(msg_ptr, json_msg.length() + 1 + bmsg_length);
        
        msg.release();
        // #ifdef DEBUG
        // std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "API_sendCMD " << jmsg.dump() << _NORMAL_CONSOLE_TEXT_ << std::endl;
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
Json uavos::andruav_servers::CAndruavCommServer::generateJSONMessage (const std::string& message_routing, const std::string& sender_name, const std::string& target_party_id, const int messageType, const std::string& message)
{

    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "generateJSONMessage " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    Json jMsg;
    jMsg["ty"] = message_routing;
    jMsg["sd"] = sender_name;
    if (!target_party_id.empty())
    {
        jMsg["tg"] = target_party_id;
    }
    else
    {
        // Inconsistent packet.... but dont enforce global packet for security reasons.
        //jMsg["ty"] = CMD_COMM_GROUP; // enforce group if party id is null.
    }
    jMsg["mt"] = messageType;
    jMsg["ms"] = message;
    

    return jMsg;
}

