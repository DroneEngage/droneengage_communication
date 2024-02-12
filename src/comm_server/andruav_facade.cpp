
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
#include "../uavos/uavos_modules_manager.hpp"
#include "../status.hpp"

#include "andruav_comm_server.hpp"
#include "andruav_facade.hpp"

using namespace uavos::andruav_servers;

void uavos::andruav_servers::CAndruavFacade::API_requestID (const std::string& target_party_id) const  
{
    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: API_requestID " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    Json jMsg = {{"C", TYPE_AndruavMessage_ID}};
    
    uavos::andruav_servers::CAndruavCommServer::getInstance().API_sendCMD (target_party_id, TYPE_AndruavMessage_RemoteExecute, jMsg);
}


/**
 * @brief send @see TYPE_AndruavMessage_ID identification message.
 * 
 * @param target_party_id 
 */
void uavos::andruav_servers::CAndruavFacade::API_sendID (const std::string& target_party_id) const 
{
    uavos::CAndruavUnitMe& andruavMe = uavos::CAndruavUnitMe::getInstance();
    uavos::ANDRUAV_UNIT_INFO&  unit_info = andruavMe.getUnitInfo();
   

    Json jMsg = 
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
        {"m1", uavos::CUavosModulesManager::getInstance().getModuleListAsJSON()}
    };
 
    uavos::ANDRUAV_UNIT_P2P_INFO& andruav_unit_p2p_info = andruavMe.getUnitP2PInfo();
    if (andruav_unit_p2p_info.p2p_connection_type != ANDRUAV_UNIT_P2P_TYPE::unknown)
    {   // P2P is available
        jMsg["p2"] = 
        {
            {"c",  andruav_unit_p2p_info.p2p_connection_type},
            {"a1", andruav_unit_p2p_info.address_1},
            {"a2", andruav_unit_p2p_info.address_2},
            {"wc", andruav_unit_p2p_info.wifi_channel},
            {"wp", andruav_unit_p2p_info.wifi_password},

            {"pa", andruav_unit_p2p_info.parent_address},
            {"pc", andruav_unit_p2p_info.parent_connection_status}
        };
    }   
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
    std::cout << "API_sendID:" <<  jMsg.dump() << std::endl;
    uavos::andruav_servers::CAndruavCommServer::getInstance().API_sendCMD (target_party_id, TYPE_AndruavMessage_ID, jMsg);
}

void uavos::andruav_servers::CAndruavFacade::API_sendCameraList(const bool reply, const std::string& target_party_id) const 
{
    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: API_requestID " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    uavos::CUavosModulesManager& module_manager = uavos::CUavosModulesManager::getInstance();
    
    Json camera_list = module_manager.getCameraList();

    const Json jMsg = 
    {
        {"R", reply},
        {"T", camera_list}
    };
        
    uavos::andruav_servers::CAndruavCommServer::getInstance().API_sendCMD (target_party_id, TYPE_AndruavMessage_CameraList, jMsg);
}




void uavos::andruav_servers::CAndruavFacade::API_sendErrorMessage (const std::string& target_party_id, const int& error_number, const int& info_type, const int& notification_type, const std::string& description) const 
{
    /*
        EN : error number  "not currently processed".
        IT : info type indicate what component is reporting the error.
        NT : sevirity and com,pliant with ardupilot.
        DS : description message.
    */
    Json message =
        {
            {"EN", error_number},
            {"IT", info_type},
            {"NT", notification_type},
            {"DS", description}
        };

    
    uavos::andruav_servers::CAndruavCommServer::getInstance().API_sendCMD (target_party_id, TYPE_AndruavMessage_Error, message);

    std::cout << std::endl << _SUCCESS_CONSOLE_BOLD_TEXT_ << "sendErrorMessage " << _NORMAL_CONSOLE_TEXT_ << description << std::endl;
    
    PLOG(plog::info) << "API_sendErrorMessage: error_number:" << std::to_string(error_number) << " info_type:" << std::to_string(info_type) << " notification_type:" << std::to_string(notification_type) << " description:" << description; 
        
    return ;
}

void uavos::andruav_servers::CAndruavFacade::API_loadTasksByScope(const ENUM_TASK_SCOPE scope, const int task_type) const
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
    

void uavos::andruav_servers::CAndruavFacade::API_loadTasksByScopeGlobal(const int task_type) const
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


void uavos::andruav_servers::CAndruavFacade::API_loadTasksByScopeAccount(const int task_type) const
{
    
    PLOG(plog::info) << "API_loadTasksByScopeAccount called"; 
    
    API_loadTask(0,
        uavos::CAndruavUnitMe::getInstance().getUnitInfo().unit_name,
        SPECIAL_NAME_ANY,
        SPECIAL_NAME_ANY,
        std::string(),
        SPECIAL_NAME_VEHICLE_RECEIVERS,
        task_type,
        true
        );
}


void uavos::andruav_servers::CAndruavFacade::API_loadTasksByScopeGroup(const int task_type) const
{
    PLOG(plog::info) << "API_loadTasksByScopeGroup called"; 
    
    API_loadTask(0,
        uavos::CAndruavUnitMe::getInstance().getUnitInfo().unit_name,
        SPECIAL_NAME_ANY,
        uavos::CAndruavUnitMe::getInstance().getUnitInfo().group_name,
        std::string(),
        SPECIAL_NAME_VEHICLE_RECEIVERS,
        task_type,
        true
        );
}


void uavos::andruav_servers::CAndruavFacade::API_loadTasksByScopePartyID(const int task_type) const
{
    PLOG(plog::info) << "API_loadTasksByScopePartyID called"; 
    
    API_loadTask(0,
        uavos::CAndruavUnitMe::getInstance().getUnitInfo().unit_name,
        uavos::CAndruavUnitMe::getInstance().getUnitInfo().party_id,
        uavos::CAndruavUnitMe::getInstance().getUnitInfo().group_name,
        std::string(),
        SPECIAL_NAME_VEHICLE_RECEIVERS,
        task_type,
        true
        );
}


void uavos::andruav_servers::CAndruavFacade::API_loadTask(const int larger_than_SID, const std::string& account_id, const std::string& party_sid, const std::string& group_name, const std::string& sender, const std::string& receiver, const int msg_type, bool is_permanent ) const
{

    PLOG(plog::info) << "API_loadTask called"; 
    
    Json message =
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

    
    uavos::andruav_servers::CAndruavCommServer::getInstance().API_sendSystemMessage (TYPE_AndruavSystem_LoadTasks, message);

    std::cout << std::endl << _SUCCESS_CONSOLE_BOLD_TEXT_ << "API_sendSystemMessage " << _NORMAL_CONSOLE_TEXT_ << message.dump() << std::endl;
    
    return ;
}


void uavos::andruav_servers::CAndruavFacade::API_sendPrepherals (const std::string& target_party_id) const 
{
    

    uavos::CAndruavUnitMe& andruavMe = uavos::CAndruavUnitMe::getInstance();
    uavos::ANDRUAV_UNIT_INFO&  unit_info = andruavMe.getUnitInfo();
   
    //uavos::STATUS& status = uavos::STATUS::getInstance();

    Json jMsg = {  };
 
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
    
    uavos::andruav_servers::CAndruavCommServer::getInstance().API_sendCMD (target_party_id, TYPE_AndruavMessage_Prepherials, jMsg);
}

/**
/// @brief tell a unit to connect to my own node.
/// @param target_party_id 
*/
void uavos::andruav_servers::CAndruavFacade::API_P2P_connectToMeshOnMyMac (const std::string& target_party_id) const 
{
    uavos::CAndruavUnitMe& andruavMe = uavos::CAndruavUnitMe::getInstance();
    uavos::ANDRUAV_UNIT_P2P_INFO& andruav_unit_p2p_info = andruavMe.getUnitP2PInfo();
    
    API_P2P_connectToMeshOnMac(target_party_id, 
                        andruav_unit_p2p_info.wifi_password,
                        andruav_unit_p2p_info.wifi_channel,
                        andruav_unit_p2p_info.address_2);
}

/**
/// @brief tell a drone to connect to a given mesh node.
/// @param target_party_id 
/// @param wifi_password 
/// @param wifi_channel 
/// @param node_mac 
*/
void uavos::andruav_servers::CAndruavFacade::API_P2P_connectToMeshOnMac (const std::string& target_party_id, const std::string& wifi_password, const uint8_t& wifi_channel, const std::string& node_mac) const 
{
    
    Json jMsg = { 
        {"a", P2P_ACTION_CONNECT_TO_MAC},
        {"b", node_mac},
        {"p", wifi_password},
        {"c", wifi_channel},

     };
 
    uavos::andruav_servers::CAndruavCommServer::getInstance().API_sendCMD (target_party_id, TYPE_AndruavMessage_P2P_ACTION, jMsg);
 
}


void uavos::andruav_servers::CAndruavFacade::API_P2P_restartMeshOnNode (const std::string& target_party_id) const 
{
    
    Json jMsg = { 
        {"a", P2P_ACTION_RESTART_TO_MAC}
     };
 
    uavos::andruav_servers::CAndruavCommServer::getInstance().API_sendCMD (target_party_id, TYPE_AndruavMessage_P2P_ACTION, jMsg);
 
}


void uavos::andruav_servers::CAndruavFacade::API_P2P_reportConnectedToParent(const std::string& target_party_id, const std::string& parent_mac) const 
{

    uavos::CAndruavUnitMe& andruavMe = uavos::CAndruavUnitMe::getInstance();
    uavos::ANDRUAV_UNIT_P2P_INFO& andruav_unit_p2p_info = andruavMe.getUnitP2PInfo();
    Json jMsg = {  
        {"a", P2P_ACTION_CONNECT_TO_MAC},
        {"p",  parent_mac},
        {"b2", andruav_unit_p2p_info.address_2}
    };
 
    uavos::andruav_servers::CAndruavCommServer::getInstance().API_sendCMD (target_party_id, TYPE_AndruavMessage_P2P_STATUS, jMsg);

}