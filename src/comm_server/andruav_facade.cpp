
#include <cstdlib>
#include <string>
#include <iostream>

#include <plog/Log.h> 
#include "plog/Initializers/RollingFileInitializer.h"


#include "../helpers/colors.hpp"
#include "../helpers/helpers.hpp"
#include "../helpers/util_rpi.hpp"

#include "../version.h"
#include "../messages.hpp"
#include "../configFile.hpp"
#include "../de_broker/de_modules_manager.hpp"
#include "../status.hpp"

#include "andruav_comm_server.hpp"
#include "andruav_facade.hpp"

using namespace de::andruav_servers;

void de::andruav_servers::CAndruavFacade::API_requestID (const std::string& target_party_id) const  
{
    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: API_requestID " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    Json_de jMsg = {{"C", TYPE_AndruavMessage_ID}};
    
    de::andruav_servers::CAndruavCommServer::getInstance().API_sendCMD (target_party_id, TYPE_AndruavMessage_RemoteExecute, jMsg);
}


/**
 * @brief send @see TYPE_AndruavMessage_ID identification message.
 * 
 * @param target_party_id 
 */
void de::andruav_servers::CAndruavFacade::API_sendID (const std::string& target_party_id) const 
{
    de::CAndruavUnitMe& andruavMe = de::CAndruavUnitMe::getInstance();
    de::ANDRUAV_UNIT_INFO&  unit_info = andruavMe.getUnitInfo();
   

    Json_de jMsg = 
    {   
        {"VT", unit_info.vehicle_type},                     // vehicle type
        {"GS", unit_info.is_gcs},                           // is gcs
        {"VR", unit_info.is_video_recording},               // is video recording
        {"B",  unit_info.is_gcs_blocked},                   // is gcs blocked
        {"FM", unit_info.flying_mode},                      // Flying mode
        {"GM", unit_info.gps_mode},                         // gps mode
        {"TP", unit_info.telemetry_protocol},               // m_telemetry_protocol
        {"C",  unit_info.manual_TX_blocked_mode},           // Remote Control RX Mode
        {"UD", unit_info.unit_name},                        // unit Name
        {"DS", unit_info.description},                      // unit Description
        {"p",  unit_info.permission},                       // permissions
        {"dv", version_string},                             // de version
        {"m1", de::comm::CUavosModulesManager::getInstance().getModuleListAsJSON()},
        {"T", get_time_usec()}                              // This is a time sync so that any time difference sent by this module can be compared.    
    };
 
    if (unit_info.is_tracking_mode)
    {
        jMsg["b"] = unit_info.is_tracking_mode;
    }
    if (unit_info.use_fcb)
    {
        jMsg["FI"] = unit_info.use_fcb;
        jMsg["AP"] = unit_info.autopilot;
    }
    if (unit_info.is_flying)
    {
        jMsg["FL"] = unit_info.is_flying;   // is flying or sinking
    }
    if (unit_info.armed_status)
    {
        jMsg["AR"] = unit_info.armed_status;    // is armed - ready to arm
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
    
    if (unit_info.swarm_follower_formation != FORMATION_NO_SWARM) // NO Formation
    {
        jMsg["n"] = unit_info.swarm_follower_formation;    
    }

    if (unit_info.swarm_leader_formation != FORMATION_NO_SWARM) // NO Formation
    {
        jMsg["o"] = unit_info.swarm_leader_formation;    
    }

    if ((!unit_info.swarm_leader_I_am_following.empty()) && (!unit_info.swarm_leader_I_am_following.length()==0))
    {
        jMsg["q"] = unit_info.swarm_leader_I_am_following;    
    }
    
    if (unit_info.flying_last_start_time > 0)
    {
        jMsg["z"] = unit_info.flying_last_start_time;    // is whisling
    }
    if (unit_info.flying_total_duration > 0)
    {
        jMsg["a"] = unit_info.flying_total_duration;    // is whisling
    }
    
    
    #ifdef DDEBUG
        std::cout << "API_sendID:" <<  jMsg.dump() << std::endl;
    #endif
    // !TODO Enhance logic here
    std::string data = de::andruav_servers::CAndruavCommServer::getInstance().API_sendCMDDummy(target_party_id, TYPE_AndruavMessage_ID, jMsg);
    de::comm::CUavosModulesManager::getInstance().processIncommingServerMessage(unit_info.party_id, TYPE_AndruavMessage_ID,  data.c_str(), data.length(), std::string());
    
    de::andruav_servers::CAndruavCommServer::getInstance().API_sendCMD (target_party_id, TYPE_AndruavMessage_ID, jMsg);
}


void de::andruav_servers::CAndruavFacade::API_sendCameraList(const bool reply, const std::string& target_party_id) const 
{
    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: API_requestID " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    de::comm::CUavosModulesManager& module_manager = de::comm::CUavosModulesManager::getInstance();
    
    Json_de camera_list = module_manager.getCameraList();

    const Json_de jMsg = 
    {
        {"R", reply},
        {"T", camera_list}
    };
        
    de::andruav_servers::CAndruavCommServer::getInstance().API_sendCMD (target_party_id, TYPE_AndruavMessage_CameraList, jMsg);
}




void de::andruav_servers::CAndruavFacade::API_sendErrorMessage (const std::string& target_party_id, const int& error_number, const int& info_type, const int& notification_type, const std::string& description) const 
{
    /*
        EN : error number  "not currently processed".
        IT : info type indicate what component is reporting the error.
        NT : sevirity and com,pliant with ardupilot.
        DS : description message.
    */
    Json_de message =
        {
            {"EN", error_number},
            {"IT", info_type},
            {"NT", notification_type},
            {"DS", description}
        };

    
    de::andruav_servers::CAndruavCommServer::getInstance().API_sendCMD (target_party_id, TYPE_AndruavMessage_Error, message);

    std::cout << std::endl << _SUCCESS_CONSOLE_BOLD_TEXT_ << "sendErrorMessage " << _NORMAL_CONSOLE_TEXT_ << description << std::endl;
    
    PLOG(plog::info) << "API_sendErrorMessage: error_number:" << std::to_string(error_number) << " info_type:" << std::to_string(info_type) << " notification_type:" << std::to_string(notification_type) << " description:" << description; 
        
    return ;
}

void de::andruav_servers::CAndruavFacade::API_loadTasksByScope(const ENUM_TASK_SCOPE scope, const int task_type) const
{
    
    PLOG(plog::info) << "LoadTasksByScope called"; 
    
    switch (scope)
    {
        case SCOPE_GLOBAL:
            API_loadTasksByScopeGlobal(task_type);
        break;
        case SCOPE_ACCOUNT:
            API_loadTasksByScopeAccount(task_type);
        break;
        case SCOPE_GROUP:
            API_loadTasksByScopeGroup(task_type);
        break;
        case SCOPE_PARTY_ID:
            API_loadTasksByScopePartyID(task_type);
        break;
    }
    return ;
}
    

void de::andruav_servers::CAndruavFacade::API_loadTasksByScopeGlobal(const int task_type) const
{
    PLOG(plog::info) << "API_loadTasksByScopeGlobal called"; 
    
    API_loadTask(0,
        SPECIAL_NAME_ANY,
        SPECIAL_NAME_ANY,
        SPECIAL_NAME_ANY,
        std::string(),
        SPECIAL_NAME_VEHICLE_RECEIVERS,
        task_type,
        true
        );
}


void de::andruav_servers::CAndruavFacade::API_loadTasksByScopeAccount(const int task_type) const
{
    
    PLOG(plog::info) << "API_loadTasksByScopeAccount called"; 
    
    API_loadTask(0,
        de::CAndruavUnitMe::getInstance().getUnitInfo().unit_name,
        SPECIAL_NAME_ANY,
        SPECIAL_NAME_ANY,
        std::string(),
        SPECIAL_NAME_VEHICLE_RECEIVERS,
        task_type,
        true
        );
}


void de::andruav_servers::CAndruavFacade::API_loadTasksByScopeGroup(const int task_type) const
{
    PLOG(plog::info) << "API_loadTasksByScopeGroup called"; 
    
    API_loadTask(0,
        de::CAndruavUnitMe::getInstance().getUnitInfo().unit_name,
        SPECIAL_NAME_ANY,
        de::CAndruavUnitMe::getInstance().getUnitInfo().group_name,
        std::string(),
        SPECIAL_NAME_VEHICLE_RECEIVERS,
        task_type,
        true
        );
}


void de::andruav_servers::CAndruavFacade::API_loadTasksByScopePartyID(const int task_type) const
{
    PLOG(plog::info) << "API_loadTasksByScopePartyID called"; 
    
    API_loadTask(0,
        de::CAndruavUnitMe::getInstance().getUnitInfo().unit_name,
        de::CAndruavUnitMe::getInstance().getUnitInfo().party_id,
        de::CAndruavUnitMe::getInstance().getUnitInfo().group_name,
        std::string(),
        SPECIAL_NAME_VEHICLE_RECEIVERS,
        task_type,
        true
        );
}


void de::andruav_servers::CAndruavFacade::API_loadTask(const int larger_than_SID, const std::string& account_id, const std::string& party_sid, const std::string& group_name, const std::string& sender, const std::string& receiver, const int msg_type, bool is_permanent ) const
{

    PLOG(plog::info) << "API_loadTask called"; 
    
    Json_de message =
        {
            {"lts", larger_than_SID},
            {"ps", party_sid},
            {"ai", account_id},
            {"gn", group_name},
            {"s", sender},
            {"r", receiver},
            {"mt", msg_type},
            {"ip", is_permanent}
        };

    
    de::andruav_servers::CAndruavCommServer::getInstance().API_sendSystemMessage (TYPE_AndruavSystem_LoadTasks, message);

    std::cout << std::endl << _SUCCESS_CONSOLE_BOLD_TEXT_ << "API_sendSystemMessage " << _NORMAL_CONSOLE_TEXT_ << message.dump() << std::endl;
    
    return ;
}

// NOT USED .... ENABLE IT
void de::andruav_servers::CAndruavFacade::API_sendPrepherals (const std::string& target_party_id) const 
{
    

    de::CAndruavUnitMe& andruavMe = de::CAndruavUnitMe::getInstance();
    de::ANDRUAV_UNIT_INFO&  unit_info = andruavMe.getUnitInfo();
   

    Json_de jMsg = {  };
 
    if (unit_info.is_tracking_mode)
    {
        jMsg["a"] = unit_info.is_tracking_mode;
    }
    if (unit_info.use_fcb)
    {
        jMsg["b"] = unit_info.use_fcb;
    }
    if (unit_info.is_flying)
    {
        jMsg["FL"] = unit_info.is_flying;   // is flying or sinking
    }
    if (unit_info.armed_status)
    {
        jMsg["AR"] = unit_info.armed_status;    // is armed
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
    if (unit_info.swarm_follower_formation != FORMATION_NO_SWARM) // NO Formation
    {
        jMsg["n"] = unit_info.swarm_follower_formation;    
    }

    if (unit_info.swarm_leader_formation != FORMATION_NO_SWARM) // NO Formation
    {
        jMsg["o"] = unit_info.swarm_leader_formation;    
    }

    if ((!unit_info.swarm_leader_I_am_following.empty()) && (!unit_info.swarm_leader_I_am_following.length()==0))
    {
        jMsg["q"] = unit_info.swarm_leader_I_am_following;    
    }
    
    de::andruav_servers::CAndruavCommServer::getInstance().API_sendCMD (target_party_id, TYPE_AndruavMessage_Prepherials, jMsg);
}



void de::andruav_servers::CAndruavFacade::API_sendCommunicationLineStatus(const std::string&  target_party_id, const bool on_off) const
{
    /*
        [ws]: bool
        [p2p]: bool
    */
    
    Json_de jMsg = {
        {
            "ws", on_off
        }
      };
 

    de::andruav_servers::CAndruavCommServer::getInstance().API_sendCMD (target_party_id, TYPE_AndruavMessage_Communication_Line_Status, jMsg);
}