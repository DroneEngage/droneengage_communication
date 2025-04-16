#include "andruav_comm_server_manager.hpp"



using namespace de::andruav_servers;


void CAndruavCommServerManager::sendMessageToCommunicationServer (const char * full_message, const std::size_t full_message_length, const bool &is_system, const bool &is_binary, const std::string &target_id, const int msg_type, const Json_de &msg_cmd )
{
    CAndruavCommServer& andruav_server = andruav_servers::CAndruavCommServer::getInstance();
    CAndruavCommServerLocal& andruav_server_local = andruav_servers::CAndruavCommServerLocal::getInstance();

    andruav_server.sendMessageToCommunicationServer (full_message, full_message_length, is_system, is_binary, target_id, msg_type, msg_cmd);
    andruav_server_local.sendMessageToCommunicationServer (full_message, full_message_length, is_system, is_binary, target_id, msg_type, msg_cmd);
}


bool CAndruavCommServerManager::isOnline()
{
    const bool status_is_online = ((de::andruav_servers::CAndruavCommServer::getInstance().getStatus()==SOCKET_STATUS_REGISTERED)
                                        || (de::andruav_servers::CAndruavCommServerLocal::getInstance().getStatus()==SOCKET_STATUS_REGISTERED));
        
    return status_is_online;
}


void CAndruavCommServerManager::uninit(bool exit_mode)
{
    CAndruavCommServer& andruav_server = andruav_servers::CAndruavCommServer::getInstance();
    CAndruavCommServerLocal& andruav_server_local = andruav_servers::CAndruavCommServerLocal::getInstance();

    andruav_server.uninit(true);
    andruav_server_local.uninit(true);
    
}


void CAndruavCommServerManager::API_sendCMD (const std::string& target_party_id, const int command_type, const Json_de& msg) const
{
    CAndruavCommServer::getInstance().API_sendCMD (target_party_id, command_type, msg);
    CAndruavCommServerLocal::getInstance().API_sendCMD (target_party_id, command_type, msg);
}



const std::string CAndruavCommServerManager::API_sendCMDDummy (const std::string& target_party_id, const int command_type, const Json_de& msg) const
{
    std::string str1 = CAndruavCommServer::getInstance().API_sendCMDDummy (target_party_id, command_type, msg);
    std::string str2 = CAndruavCommServerLocal::getInstance().API_sendCMDDummy (target_party_id, command_type, msg);
    
    if (str1.empty() )
    {
        return str2;
    }

    return str1;
}


void CAndruavCommServerManager::turnOnOff(const bool on_off, const uint32_t duration_seconds)
{
    CAndruavCommServer::getInstance().turnOnOff (on_off, duration_seconds);
    CAndruavCommServerLocal::getInstance().turnOnOff (on_off, duration_seconds);
}