
#include <cstdlib>
#include <string>
#include <iostream>



#include "../helpers/colors.hpp"
#include "../helpers/helpers.hpp"
#include "../helpers/util_rpi.hpp"


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
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: API_requestID " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    Json jMsg = {{"C", TYPE_AndruavMessage_ID}};
    
    uavos::andruav_servers::CAndruavCommServer::getInstance().API_sendCMD (target_party_id, TYPE_AndruavMessage_RemoteExecute, jMsg);
}


void uavos::andruav_servers::CAndruavFacade::API_sendID (const std::string& target_party_id) const 
{
    uavos::CConfigFile& cConfigFile = uavos::CConfigFile::getInstance();
    const Json& jsonConfig = cConfigFile.GetConfigJSON();


    uavos::CAndruavUnitMe& m_andruavMe = uavos::CAndruavUnitMe::getInstance();
    uavos::ANDRUAV_UNIT_INFO&  unit_info = m_andruavMe.getUnitInfo();
   

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
        {"UD", jsonConfig["unitID"]},                       // unit Name
        {"DS", jsonConfig["unitDescription"]},              // unit Description
        {"p",  unit_info.permission},                       // permissions
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
    if (unit_info.swarm_leader_formation != FORMATION_NO_SWARM) // NO Formation
    {
        jMsg["o"] = unit_info.swarm_leader_formation;    
    }

    if ((!unit_info.swarm_leader_I_am_following.empty()) && (!unit_info.swarm_leader_I_am_following.length()==0))
    {
        jMsg["q"] = unit_info.swarm_leader_I_am_following;    
    }
    
    uavos::andruav_servers::CAndruavCommServer::getInstance().API_sendCMD (target_party_id, TYPE_AndruavMessage_ID, jMsg);
}

void uavos::andruav_servers::CAndruavFacade::API_sendCameraList(const bool reply, const std::string& target_party_id) const 
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
    
    return ;
}

void uavos::andruav_servers::CAndruavFacade::API_loadTasksByScope(const ENUM_TASK_SCOPE scope, const int task_type) const
{
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

    
    uavos::andruav_servers::CAndruavCommServer::getInstance().API_sendSystemMessage (TYPE_AndruavSystem_LoadTasks, message.dump());

    std::cout << std::endl << _SUCCESS_CONSOLE_BOLD_TEXT_ << "API_sendSystemMessage " << _NORMAL_CONSOLE_TEXT_ << message.dump() << std::endl;
    
    return ;
}


void uavos::andruav_servers::CAndruavFacade::API_sendPrepherals (const std::string& target_party_id) const 
{
    

    uavos::CAndruavUnitMe& m_andruavMe = uavos::CAndruavUnitMe::getInstance();
    uavos::ANDRUAV_UNIT_INFO&  unit_info = m_andruavMe.getUnitInfo();
   
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