#include <plog/Log.h> 
#include "plog/Initializers/RollingFileInitializer.h"


#include "../helpers/colors.hpp"
#include "../helpers/helpers.hpp"
#include "../global.hpp"
#include "../messages.hpp"
#include "../localConfigFile.hpp"
#include "andruav_parser.hpp"
#include "andruav_facade.hpp"
#include "andruav_comm_server.hpp"
#include "../de_general_mission_planner/mission_manager_base.hpp"
#include "../status.hpp"


void de::andruav_servers::CAndruavParser::parseCommand (const std::string& sender_party_id, const int& command_type, const Json_de& jsonMessage)
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
            if (command.contains("AR") == true) unit_info.armed_status = command["AR"].get<int>();
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

        case TYPE_AndruavMessage_Sync_EventFire:
        {
            // Events received form another units.

            const Json_de cmd = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD];
            
            if (validateField(cmd, "d",Json_de::value_t::string)) 
            {
                // string droneengage event format.
                mission::CMissionManagerBase::getInstance().fireWaitingCommands(cmd["d"].get<std::string>());
            }
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
                de::andruav_servers::CAndruavCommServer::getInstance().turnOnOff(ws_on_off, ws_duration);
            }

            // Message can be handled by other modules.
        }
        break;

        case TYPE_AndruavMessage_Upload_DE_Mission:
        {
            /**
             * @brief This is an important function that handles DE Mission.
             * TYPE_AndruavMessage_Upload_DE_Mission is handled by mavlink amd 
             * can be handled by any module that wants to execute missions.
             * 
             */

            de::CAndruavUnitMe& andruavMe = de::CAndruavUnitMe::getInstance();
            de::ANDRUAV_UNIT_INFO&  unit_info = andruavMe.getUnitInfo();
  
            if (unit_info.is_gcs_blocked) break ;

            const Json_de command = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD];
            
            if (unit_info.is_gcs_blocked) break ;
                
            if ((!is_system) && ((permission & PERMISSION_ALLOW_GCS_WP_CONTROL) != PERMISSION_ALLOW_GCS_WP_CONTROL)) \
            {
                std::cout << _INFO_CONSOLE_BOLD_TEXT << "DroneEngage Mission-Upload: "  << _ERROR_CONSOLE_BOLD_TEXT_ << "Permission Denied." << _NORMAL_CONSOLE_TEXT_ << std::endl;
                break;
            }
                
                
                
            /*
                a : std::string serialized mission file
            */
            if (!validateField(command, "j", Json_de::value_t::object)) 
            {
                andruav_servers::CAndruavFacade::getInstance().API_sendErrorMessage(std::string(), 0, ERROR_3DR, NOTIFICATION_TYPE_ERROR, "Bad input plan file");

                break ;
            }

            if (validateField(command, "e", Json_de::value_t::boolean))
            {
                //de::mission::CMissionManagerBase::getInstance().clearModuleMissionItems();
            }

                

            const Json_de plan_object = command ["j"];

            de::mission::CMissionManagerBase::getInstance().extractPlanModule(plan_object);
                    
            #ifdef DDEBUG
                std::cout << _INFO_CONSOLE_BOLD_TEXT << "DroneEngage Mission-Upload: "  << plan_object.dump() << std::endl;
            #endif
        }
        break; 
    }
}


void de::andruav_servers::CAndruavParser::parseRemoteExecuteCommand (const std::string& sender_party_id, const Json_de& jsonMessage)
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

        case RemoteCommand_CLEAR_WAY_POINTS_FROM_FCB:
            
            if (unit_info.is_gcs_blocked) break ;
            
            if ((!is_system) && ((permission & PERMISSION_ALLOW_GCS_WP_CONTROL) != PERMISSION_ALLOW_GCS_WP_CONTROL))
            {
                std::cout << _INFO_CONSOLE_BOLD_TEXT << "CLEAR_WAY_POINTS_FROM_FCB: "  << _ERROR_CONSOLE_BOLD_TEXT_ << "Permission Denied." << _NORMAL_CONSOLE_TEXT_ << std::endl;
                break;
            }
                
            de::mission::CMissionManagerBase::getInstance().clearModuleMissionItems();
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