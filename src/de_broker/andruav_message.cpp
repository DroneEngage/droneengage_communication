#include <cstdlib>
#include <string>
#include <iostream>

#include "../helpers/colors.hpp"
#include "../helpers/helpers.hpp"
#include "../messages.hpp"


#include "andruav_message.hpp"




using namespace de::comm;


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
Json_de CAndruavMessage::generateJSONMessage (const std::string& message_routing, const std::string& sender_name, const std::string& target_party_id, const int messageType, const Json_de& message) const
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
Json_de CAndruavMessage::generateJSONSystemMessage (const int messageType, const Json_de& message) const
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
            