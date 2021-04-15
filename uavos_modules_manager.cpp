#include <iostream>


#include <exception>
#include <typeinfo>
#include <stdexcept>


#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 


#include "./helpers/colors.hpp"
#include "./helpers/helpers.hpp"


#include "messages.hpp"
#include "andruav_unit.hpp"
#include "udpCommunicator.hpp"
#include "configFile.hpp"
#include "andruav_comm_server.hpp"
#include "uavos_modules_manager.hpp"


uavos::CUavosModulesManager::~CUavosModulesManager()
{

}


/**
 * creates JSON message that identifies Module
**/
Json uavos::CUavosModulesManager::createJSONID (const bool& reSend)
{
    try
    {
    
        uavos::CConfigFile& cConfigFile = uavos::CConfigFile::getInstance();
        const Json& jsonConfig = cConfigFile.GetConfigJSON();
        Json jsonID;        
        
        jsonID[INTERMODULE_COMMAND_TYPE] =  CMD_TYPE_INTERMODULE;
        jsonID[ANDRUAV_PROTOCOL_MESSAGE_TYPE] =  TYPE_AndruavModule_ID;
        Json ms;
        
        ms["a"] = jsonConfig["module_id"];
        ms["b"] = jsonConfig["module_class"];
        ms["c"] = jsonConfig["module_messages"];
        ms["d"] = Json();
        ms["e"] = jsonConfig["module_key"]; 
        ms["f"] = 
        {
            {"sd", jsonConfig["partyID"]},
            {"gr", jsonConfig["groupID"]}
        };
        
        // this is NEW in communicator and could be ignored by current UAVOS modules.
        ms["z"] = reSend;

        jsonID[ANDRUAV_PROTOCOL_MESSAGE_CMD] = ms;
        
        return jsonID;

                /* code */
    }
    catch (...)
    {
        //https://stackoverflow.com/questions/315948/c-catching-all-exceptions/24142104
        std::exception_ptr p = std::current_exception();
        std::clog <<(p ? p.__cxa_exception_type()->name() : "null") << std::endl;
        
        return Json();
    }

}


/**
 * @brief 
 * Handels CMD_TYPE_INTERMODULE messages.
 * CMD_TYPE_INTERMODULE message types contain many commands. most important is TYPE_AndruavModule_ID
 * Other Andruav based commands can be sent using this type of messages if uavos module wants uavos communicator to process 
 * the message before forwarding it. Although it is not necessary to forward the message to Andruav-Server.
 * @param jsonMessage Json object message.
 * @param port        uavos module port 
 * @param address     uavos module address
 * @param forward     true to forward to Andruav Server
 */
void uavos::CUavosModulesManager::parseIntermoduleMessage (Json& jsonMessage, struct sockaddr_in * ssock, bool& forward)
{
    // Intermodule Message
    const int mt = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_TYPE].get<int>();
    //forward = false;
    const Json ms = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD];
    switch (mt)
    {

        case TYPE_AndruavModule_ID:
        {
            
            const uint64_t &now = get_time_usec();
            
            // array of message IDs
            const Json& message_array       = ms["c"]; 

            MODULE_ITEM_TYPE * module_item;
            const std::string& module_id = std::string(ms["a"].get<std::string>()); 
            /**
             * @brief insert module in @param m_modules_list
             * this is the main list of modules.
             */
            auto search2 = m_modules_list.find(module_id);
            if (search2 == m_modules_list.end()) 
            {
                // New Module

                module_item = new MODULE_ITEM_TYPE();
                module_item->module_key         = ms["e"].get<std::string>();
                module_item->module_id          = module_id;
                module_item->module_class       = ms["b"].get<std::string>(); // fcb, video, ...etc.
                module_item->modules_features   = ms["d"];
            
                struct sockaddr_in * module_address = new (struct sockaddr_in)();  
                memcpy(module_address, ssock, sizeof(struct sockaddr_in)); 
                
                module_item->m_module_address = std::unique_ptr<struct sockaddr_in>(module_address);
                
                m_modules_list.insert(std::make_pair(module_item->module_id, std::shared_ptr<MODULE_ITEM_TYPE>(module_item)));


            }
            else
            {
                module_item = search2->second.get();
                
                // Update Module Info
                if ((module_item->module_last_access_time!=0)
               && (now - module_item->module_last_access_time >= Module_TIME_OUT))
                {
                    //Event Module Restored
                }
            }

            module_item->module_last_access_time = now;

            
            // insert message callback
            const int messages_length = message_array.size(); 
            for (int i=0; i< messages_length; ++i)
            {
                /**
                 * @brief 
                 * select list of a given message id.
                 * * &v should be by reference to avoid making fresh copy.
                 */
                std::vector<std::string> &v = m_module_messages[message_array[i].get<int>()];
                if (std::find(v.begin(), v.end(), module_id) == v.end())
                {
                    /**
                     * @brief 
                     * add module in the callback list.
                     * when this message is received from andruav-server it should be 
                     * forwarded to this list.
                     */
                    v.push_back(module_item->module_id);
                }
            }

            if (validateField(ms, "z", Json::value_t::boolean))
            {
                if (ms["z"].get<bool>() == true)
                {
                    const Json &msg = createJSONID(false);
                    struct sockaddr_in module_address = *module_item->m_module_address.get();  
                    //memcpy(module_address, ssock, sizeof(struct sockaddr_in)); 
                
                    uavos::comm::CUDPCommunicator::getInstance().SendJMSG(msg.dump(), &module_address);
                }
            }
        }
        break;

        case TYPE_AndruavResala_ID:
        {
            uavos::CAndruavUnitMe& m_andruavMe = uavos::CAndruavUnitMe::getInstance();
            uavos::ANDRUAV_UNIT_INFO&  unit_info = m_andruavMe.getUnitInfo();
            
            unit_info.vehicle_type              = ms["VT"].get<int>();
            unit_info.flying_mode               = ms["FM"].get<int>();
            unit_info.gps_mode                  = ms["GM"].get<int>();
            unit_info.use_fcb                   = ms["FI"].get<bool>();
            unit_info.is_armed                  = ms["AR"].get<bool>();
            unit_info.is_flying                 = ms["FL"].get<bool>();
            unit_info.telemetry_protocol        = ms["TP"].get<int>();
            unit_info.flying_last_start_time    = ms["z"].get<long long>();
            unit_info.flying_total_duration     = ms["a"].get<long long>();
            unit_info.is_tracking_mode          = ms["b"].get<bool>();
            unit_info.manual_TX_blocked_mode    = ms["C"].get<int>();
            unit_info.is_gcs_blocked            = ms["B"].get<bool>();

            uavos::andruav_servers::CAndruavCommServer& commServer = uavos::andruav_servers::CAndruavCommServer::getInstance();
            commServer.API_sendID("");
        }
        break;

    }

}


void uavos::CUavosModulesManager::processIncommingServerMessage (const std::string& sender_party_id, const int& command_type, const Json& jsonMessage)
{
    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: processIncommingServerMessage " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    std::vector<std::string> &v = m_module_messages[command_type];
    for(std::vector<std::string>::iterator it = v.begin(); it != v.end(); ++it) 
    {
        std::cout << *it << std::endl;
        auto search2 = m_modules_list.find(*it);
        if (search2 == m_modules_list.end()) 
        {
            // module not available
            std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Module " << *it  << " for message " << command_type << " is not available" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            
            continue;
        }
        else
        {
            
            MODULE_ITEM_TYPE * module_item = search2->second.get();        
            forwardMessageToModule (jsonMessage, module_item);
        }
    }

    return ;
}


/**
 * @brief forward a message from Andruav Server to a module.
 * Normally this module is subscribed in this message id.
 * 
 * @param jsonMessage 
 * @param module_item 
 */
void uavos::CUavosModulesManager::forwardMessageToModule (const Json& jsonMessage, const MODULE_ITEM_TYPE * module_item)
{
    //const Json &msg = createJSONID(false);
    struct sockaddr_in module_address = *module_item->m_module_address.get();  
                
    uavos::comm::CUDPCommunicator::getInstance().SendJMSG(jsonMessage.dump(), &module_address);

    return ;
}