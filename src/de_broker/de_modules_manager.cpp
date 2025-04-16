
#include <iostream>

#include <exception>
#include <typeinfo>
#include <stdexcept>

#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#include <plog/Log.h> 
#include "plog/Initializers/RollingFileInitializer.h"

#include "../helpers/colors.hpp"
#include "../helpers/helpers.hpp"


#include "../messages.hpp"
#include "./udpCommunicator.hpp"
#include "../configFile.hpp"
#include "../localConfigFile.hpp"
#include "../comm_server/andruav_unit.hpp"
#include "../comm_server/andruav_comm_server_manager.hpp"
#include "../comm_server/andruav_facade.hpp"
#include "../comm_server/andruav_auth.hpp"
#include "../de_broker/de_modules_manager.hpp"
#include "../de_general_mission_planner/mission_manager_base.hpp"
#include "andruav_message.hpp"




std::thread t;

static std::mutex g_i_mutex; 
static std::mutex g_i_mutex_process; 
static std::mutex g_i_mutex_process2; 


void de::comm::CUavosModulesManager::onReceive (const char * message, int len, struct sockaddr_in * sock)
{
    #ifdef DDEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "#####DEBUG:" << message << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    // Create a MessageWithSocket structure: : use std::move to avoid unnecessary copying
    MessageWithSocket msgWithSocket;
    msgWithSocket.message = std::move(std::string(message, len));
    msgWithSocket.socket = *sock; // Copy the socket information

    // Enqueue the message with its socket information
    m_buffer.enqueue(msgWithSocket);

}


bool de::comm::CUavosModulesManager::init (const std::string host, int listenningPort, int chunkSize)
{
    std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG:" << _NORMAL_CONSOLE_TEXT_ << std::endl;

    m_consumerThread = std::thread(&CUavosModulesManager::consumerThreadFunc, this);
    
    cUDPClient.init (host.c_str(), listenningPort, chunkSize);

    cUDPClient.start();

    return true;
}


void de::comm::CUavosModulesManager::uninit ()
{
    #ifdef DEBUG
	    std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: Stop Threads Killed" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    m_exit = true;
    cUDPClient.stop();
    const std::string WAKE_UP_SIGNAL = "WAKE_UP";
    m_buffer.enqueue({WAKE_UP_SIGNAL, sockaddr_in()}); // Enqueue an empty message to wake up the consumer

    // Wait for the consumer thread to finish
    if (m_consumerThread.joinable()) {
        m_consumerThread.join();
    }
}

            
void de::comm::CUavosModulesManager::consumerThreadFunc() 
{
    try
    {
        while (!m_exit) {
            MessageWithSocket msgWithSocket = m_buffer.dequeue();

            // Check if the message is empty (this could happen if the buffer is empty and the thread is exiting)
            if (msgWithSocket.message.empty()) {
                continue; // Skip processing and check the exit condition again
            }
            
            const std::size_t len = msgWithSocket.message.length();
            const char* c_str = msgWithSocket.message.c_str();
            
            parseIntermoduleMessage(c_str, len, &msgWithSocket.socket);

            if (m_OnReceive!= nullptr) m_OnReceive(c_str, len);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "consumerThreadFunc:" << e.what() << '\n';
    }
        
    
}

void de::comm::CUavosModulesManager::defineModule (
                 std::string module_class,
                 std::string module_id,
                 std::string module_key,
                 std::string module_version,
                 std::string party_id,
                 std::string group_id
            ) 
{
    m_module_class = module_class;
    m_module_id = module_id;
    m_module_key = module_key;
    m_module_version = module_version;
    m_party_id = party_id;
    m_group_id = group_id;
    
    return ;
}

/**
 * @brief creates JSON message that identifies Module
 * @details generates JSON message that identifies module
 * 'a': module_id
 * 'b': module_class. fixed "comm"
 * 'c': module_messages. can be updated from config file.
 * 'd': module_features. empty as it is communicator. extra can be added.
 * 'e': module_key. uniqueley identifies this instance and can be set in config file.
 * 'f': ONLY SENT BY UAVOS_COMMUNICATOR which contains partyID & GroupID
 * 'z': resend request flag
 * @param reSend if true then module should reply with module JSONID
 * @return const Json 
 */
Json de::comm::CUavosModulesManager::createJSONID (const bool& reSend)
{
    try
    {
    
        
        Json jsonID;        
        
        jsonID[INTERMODULE_ROUTING_TYPE]        =  CMD_TYPE_INTERMODULE;
        jsonID[ANDRUAV_PROTOCOL_MESSAGE_TYPE]   =  TYPE_AndruavModule_ID;
        
        Json ms;
        
        ms[JSON_INTERMODULE_MODULE_ID]              = m_module_id;
        ms[JSON_INTERMODULE_MODULE_CLASS]           = m_module_class;
        ms[JSON_INTERMODULE_MODULE_MESSAGES_LIST]   = ""; // module_messages
        ms[JSON_INTERMODULE_MODULE_FEATURES]        = Json();
        ms[JSON_INTERMODULE_MODULE_KEY]             = m_module_key; 
        ms[JSON_INTERMODULE_PARTY_RECORD]           = 
                    {
                        {"sd", m_party_id},
                        {"gr", m_group_id}
                    };
        
        // this is NEW in communicator and could be ignored by current DroneEngage modules.
        ms[JSON_INTERMODULE_SOCKET_STATUS]          = andruav_servers::CAndruavCommServer::getInstance().getStatus();
        ms[JSON_INTERMODULE_RESEND]                 = reSend;

        ms[JSON_INTERMODULE_TIMESTAMP_INSTANCE]     = m_instance_time_stamp;
        jsonID[ANDRUAV_PROTOCOL_MESSAGE_CMD]        = ms;
        
        return jsonID;
    }
    catch (...)
    {
        //https://stackoverflow.com/questions/315948/c-catching-all-exceptions/24142104
        std::exception_ptr p = std::current_exception();
        std::clog <<(p ? p.__cxa_exception_type()->name() : "null") << std::endl;
        
        PLOG(plog::error)<<(p ? p.__cxa_exception_type()->name() : "null") ; 
        
        return Json();
    }
}


/**
* @brief update DroneEngage vehicle feature based on module features.
* ex: "d" : [ "C", "V" ]
* @param module_features 
* 
* @return true if a feature has been updated
* @return false if features are the samme
*/
bool de::comm::CUavosModulesManager::updateUavosPermission (const Json& module_features)
{
    CAndruavUnitMe& andruav_unit_me = CAndruavUnitMe::getInstance();
    bool updated = false;
    //const int&  len = module_features.size();
    for (const auto feature : module_features)
    {
        const std::string& feature_item = feature.get<std::string>(); //module_features[i].get<std::string>();
        ANDRUAV_UNIT_INFO& andruav_unit_info = andruav_unit_me.getUnitInfo();
        if (feature_item.compare("T") ==0)
        {
            if (andruav_unit_info.permission[4]=='T') break;
            andruav_unit_info.permission[4] = 'T';
            updated = true;
        }
        else if (feature_item.compare("R") ==0)
        {
            if (andruav_unit_info.permission[6]=='R') break;
            andruav_unit_info.permission[6] = 'R';
            updated = true;
        }
        else if (feature_item.compare("V") ==0)
        {
            if (andruav_unit_info.permission[8]=='V') break;
            andruav_unit_info.permission[8] = 'V';
            updated = true;
        }
        else if (feature_item.compare("C") ==0)
        {
            if (andruav_unit_info.permission[10]=='C') break;
            andruav_unit_info.permission[10] = 'C';
            updated = true;
        }
        
    }

    return updated;
}



/**
* @details Camera module should send a complete list of camera devices.
* Any missing camera is one disappeared most likely the module restarted 
* and generated new camera device ids
* 
* @param module_id 
*/
void de::comm::CUavosModulesManager::cleanOrphanCameraEntries (const std::string& module_id, const uint64_t time_now)
{
    auto camera_module = m_camera_list.find(module_id);
    if (camera_module == m_camera_list.end()) {
        return; // Module not found, nothing to do
    }

    // Get the camera entry list for the module
    auto* camera_entry_list = camera_module->second.get();
    if (!camera_entry_list) {
        return; // Null pointer, nothing to do
    }

    // Collect orphaned entries and remove them
    for (auto it = camera_entry_list->begin(); it != camera_entry_list->end(); ) {
        const auto& camera_entry = it->second;
        if (camera_entry->module_last_access_time < time_now) {
            // Erase the orphaned entry and update the iterator
            it = camera_entry_list->erase(it);
        } else {
            ++it; // Move to the next entry
        }
    }
}


/**
* @brief Update camera list.
* Adding available camera devices exists in different camera modules.
* 
* @details Update camera list.
* Adding available camera devices exists in different camera modules.
* RX MSG: {
*    "ms" : {
*        "a" : "HorusEye1",
*        "b" : "camera",
*        "c" : [ 1005, 1041, 1021 ],
*        "d" : [ "C", "V" ],
*        "e" : "E289FEE7-FDAD-44EF-A257-C9A36DDD6BE7",
*        "m" : [
*            {
*                "active" : 0,
*                "id" : "G59d8d78965966a1a449b44b1",
*                "ln" : "Droidcam#0",
*                "p" : 2,
*                "r" : false,
*                "v" : true
*            },
*            {
*                "active" : 0,
*                "id" : "G207ac06d13bf7f2756f2fc51",
*                "ln" : "Dummy video device (0x0000)#1",
*                "p" : 2,
*                "r" : false,
*                "v" : true
*            },
*            {
*                "active" : 0,
*                "id" : "G69058c165ac352104cef76d9",
*                "ln" : "Dummy video device (0x0001)#2",
*                "p" : 2,
*                "r" : false,
*                "v" : true
*            },
*            {
*                "active" : 0,
*                "id" : "G65a44b9276d1e51e59658bc",
*                "ln" : "Dummy video device (0x0002)#3",
*                "p" : 2,
*                "r" : false,
*                "v" : true
*            }
*            ],
*        "z" : false
*        },
*    "message_type" : 9100,
*    "ty" : "uv"
* }
* @param msg_cmd 
*/
void de::comm::CUavosModulesManager::updateCameraList(const std::string& module_id, const Json& msg_cmd)
{

    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: updateCameraList " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    // Check if Module "Camera_ModuleName" is listed.
    auto camera_module = m_camera_list.find(module_id);
    if (camera_module == m_camera_list.end()) 
    {
        // Module Not found in camera list
        #ifdef DEBUG
            std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: updateCameraList // Module Not found in camera list" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif

        auto pcamera_entries = std::make_unique<std::map<std::string, std::unique_ptr<MODULE_CAMERA_ENTRY>>>();
        m_camera_list.emplace(module_id, std::move(pcamera_entries));
    }

    // Retrieve list of camera entries of this module.
    camera_module = m_camera_list.find(module_id);

    auto& camera_entry_list = *camera_module->second;

    // List of camera devices in a camera module recieved by intermodule message.
    const uint64_t now_time = get_time_usec();
    
    // Check if camera list is available
    if (msg_cmd.contains("m"))
    {

        const Json& camera_array = msg_cmd["m"];

        // iterate over camera devices in recieved json message.
        for (const auto& jcamera_entry : camera_array)
        {
            
            // camera device id
            const std::string& camera_entry_id = jcamera_entry["id"].get<std::string>();
            
            
            // Check if the camera entry already exists
            auto camera_entry_record = camera_entry_list.find(camera_entry_id);
            if (camera_entry_record == camera_entry_list.end()) 
            {
                // camera entry not listed in cameras list of submodule
                MODULE_CAMERA_ENTRY * camera_entry = new MODULE_CAMERA_ENTRY();
                camera_entry->module_id  = module_id;
                camera_entry->global_index = camera_entry_id;
                camera_entry->logical_name = jcamera_entry["ln"].get<std::string>();
                camera_entry->is_recording = jcamera_entry["r"].get<bool>();
                camera_entry->is_camera_avail = jcamera_entry["v"].get<bool>();
                camera_entry->is_camera_streaming = jcamera_entry["active"].get<int>();
                camera_entry->camera_type = jcamera_entry["p"].get<int>();
                camera_entry->camera_specification = jcamera_entry["s"].get<int>();
                camera_entry->module_last_access_time = now_time;
                camera_entry->updates = true;
                
                // Insert the new camera entry into the list
                camera_entry_list.emplace(camera_entry_id, std::move(camera_entry));
            
            }
            else
            {
                //camera listed
                
                MODULE_CAMERA_ENTRY * camera_entry = camera_entry_record->second.get();
                camera_entry->module_id  = module_id;
                camera_entry->global_index = camera_entry_id;
                camera_entry->logical_name = jcamera_entry["ln"].get<std::string>();
                camera_entry->is_recording = jcamera_entry["r"].get<bool>();
                camera_entry->is_camera_avail = jcamera_entry["v"].get<bool>();
                camera_entry->is_camera_streaming = jcamera_entry["active"].get<int>();
                camera_entry->camera_type = jcamera_entry["p"].get<int>();
                camera_entry->camera_specification = jcamera_entry["s"].get<int>();
                
                camera_entry->module_last_access_time = now_time;
                camera_entry->updates = true;

            }
        }
    }

    cleanOrphanCameraEntries(module_id, now_time);
}


Json de::comm::CUavosModulesManager::getCameraList() const
{
    Json camera_list = Json::array();

    // Iterate over each camera module
    for (const auto& [module_id, camera_entry_list_ptr] : m_camera_list) {
        // Check if the camera entry list is valid
        if (!camera_entry_list_ptr) {
            continue; // Skip invalid entries
        }

        // Iterate over each camera entry in the module
        for (const auto& [entry_id, camera_entry_ptr] : *camera_entry_list_ptr) {
            if (!camera_entry_ptr) {
                continue; // Skip invalid entries
            }

            // Create a JSON object for the camera entry
            Json json_camera_entry = {
                {"v", camera_entry_ptr->is_camera_avail},
                {"ln", camera_entry_ptr->logical_name},
                {"id", camera_entry_ptr->global_index},
                {"active", camera_entry_ptr->is_camera_streaming},
                {"r", camera_entry_ptr->is_recording},
                {"p", camera_entry_ptr->camera_type},
                {"s", camera_entry_ptr->camera_specification}
            };

            // Add the JSON object to the list
            camera_list.push_back(std::move(json_camera_entry));
        }
    }

    return camera_list;

}


bool de::comm::CUavosModulesManager::updateModuleSubscribedMessages(const std::string& module_id, const Json& message_array)
{
    bool new_module = false;

    // Iterate over each message ID in the JSON array
    for (const auto& message : message_array) {
        // Get the message ID and ensure it's an integer
        const int message_id = message.get<int>();

        // Get the list of modules subscribed to this message
        std::vector<std::string>& module_list = m_module_messages[message_id];

        // Check if the module is already in the list
        if (std::find(module_list.begin(), module_list.end(), module_id) == module_list.end()) {
            // Add the module to the list
            module_list.push_back(module_id);
            new_module = true;
        }
    }

    return new_module;
}


/**
 * @brief  Communicate with @link andruav_servers::CAndruavAuthenticator @endlink to validate hardware status
 * 
 * @param module_item 
 */
void de::comm::CUavosModulesManager::checkLicenseStatus (MODULE_ITEM_TYPE * module_item)
{
    andruav_servers::CAndruavAuthenticator &auth = andruav_servers::CAndruavAuthenticator::getInstance();

    if (auth.isAuthenticationOK())
    {
        if (auth.doValidateHardware(module_item->hardware_serial, module_item->hardware_type))
        {
            std::cout << std::endl << _LOG_CONSOLE_BOLD_TEXT << "Module License " << _SUCCESS_CONSOLE_TEXT_ << "OK " <<  _SUCCESS_CONSOLE_BOLD_TEXT_ << module_item->module_id << _NORMAL_CONSOLE_TEXT_ << std::endl;
            PLOG(plog::info)<< "Module License OK: " << module_item->module_id ;

            module_item->licence_status = ENUM_LICENCE::LICENSE_VERIFIED_OK;
        }
        else
        {
            std::cout << std::endl << _LOG_CONSOLE_BOLD_TEXT << "Module License " << _ERROR_CONSOLE_BOLD_TEXT_ <<  "INVALID " << module_item->module_id<< _NORMAL_CONSOLE_TEXT_ << std::endl;
            PLOG(plog::error)<< "Module License Invalid: " << module_item->module_id ;

            module_item->licence_status = ENUM_LICENCE::LICENSE_VERIFIED_BAD;
            andruav_servers::CAndruavFacade::getInstance().API_sendErrorMessage(std::string(), 0, ERROR_TYPE_ERROR_MODULE, NOTIFICATION_TYPE_ALERT, std::string("Module " + module_item->module_id + " is not allowed to run."));
        }
    }
    else
    {
        PLOG(plog::warning)<< "Module License " << module_item->module_id << " could not been verified";
        
        module_item->licence_status = ENUM_LICENCE::LICENSE_NOT_VERIFIED;
    }
}

/**
* @brief handle TYPE_AndruavModule_ID messages.
* Add/Update module definitions.
* @param msg_cmd 
* @param ssock 
* 
* @return true module has been added.
* @return false no new modules.
*/
bool de::comm::CUavosModulesManager::handleModuleRegistration (const Json& msg_cmd, const struct sockaddr_in* ssock)
{

    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: handleModuleRegistration: " << msg_cmd.dump() << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    const std::lock_guard<std::mutex> lock(g_i_mutex);

    bool updated = false;

    const uint64_t &now = get_time_usec();
            
    // array of message IDs
    
    MODULE_ITEM_TYPE * module_item;
    const std::string& module_id = std::string(msg_cmd[JSON_INTERMODULE_MODULE_ID].get<std::string>()); 
    /**
    * @brief insert module in @param m_modules_list
    * this is the main list of modules.
    */
    
    const Json& message_array = msg_cmd[JSON_INTERMODULE_MODULE_MESSAGES_LIST]; 
        
    auto module_entry = m_modules_list.find(module_id);
    if (module_entry == m_modules_list.end()) 
    {
        // New Module not registered in m_modules_list

        module_item = new MODULE_ITEM_TYPE();
        module_item->module_key         = msg_cmd[JSON_INTERMODULE_MODULE_KEY].get<std::string>();
        module_item->module_id          = module_id;
        module_item->module_class       = msg_cmd[JSON_INTERMODULE_MODULE_CLASS].get<std::string>(); // fcb, video, ...etc.
        module_item->modules_features   = msg_cmd[JSON_INTERMODULE_MODULE_FEATURES];
        if (msg_cmd.contains(JSON_INTERMODULE_TIMESTAMP_INSTANCE))
        {   
            module_item->time_stamp         = msg_cmd[JSON_INTERMODULE_TIMESTAMP_INSTANCE].get<std::time_t>();
        }
        if (msg_cmd.contains(JSON_INTERMODULE_HARDWARE_ID))
        {
            module_item->hardware_serial    = msg_cmd[JSON_INTERMODULE_HARDWARE_ID];
            module_item->hardware_type      = msg_cmd[JSON_INTERMODULE_HARDWARE_TYPE].get<int>();
            
            checkLicenseStatus(module_item);
        }
        else
        {
            module_item->licence_status = ENUM_LICENCE::LICENSE_NO_DATA;
        }

        if (msg_cmd.contains(JSON_INTERMODULE_VERSION))
        {
            module_item->version = msg_cmd[JSON_INTERMODULE_VERSION].get<std::string>();;
        }
        else
        {
            module_item->version = std::string("na");
        }

        struct sockaddr_in * module_address = new (struct sockaddr_in)();  
        memcpy(module_address, ssock, sizeof(struct sockaddr_in)); 
                
        module_item->m_module_address = std::unique_ptr<struct sockaddr_in>(module_address);
                
        m_modules_list.insert(std::make_pair(module_item->module_id, std::unique_ptr<MODULE_ITEM_TYPE>(module_item)));
        
        // insert message callback ... Module cannot update messages ids during running.
        updated |= updateModuleSubscribedMessages(module_id, message_array);

        PLOG(plog::info)<<"Module Adding: " << module_item->module_id ; 
        
    }
    else
    {
        module_item = module_entry->second.get();
        module_item->is_dead = false;

        struct sockaddr_in *module_address = module_item->m_module_address.get();
        memcpy(module_address, ssock, sizeof(struct sockaddr_in)); 

        // Update Module Info

        if ((msg_cmd.contains(JSON_INTERMODULE_TIMESTAMP_INSTANCE)) && (module_item->time_stamp != msg_cmd[JSON_INTERMODULE_TIMESTAMP_INSTANCE].get<std::time_t>()))
        {
            // module restarted
            
            if (msg_cmd.contains(JSON_INTERMODULE_VERSION))
            {
                module_item->version = msg_cmd[JSON_INTERMODULE_VERSION].get<std::string>();;
            }
            else
            {
                module_item->version = std::string("na");
            }

            //MODULE HAS BEEN RESTARTED
            module_item->time_stamp = msg_cmd[JSON_INTERMODULE_TIMESTAMP_INSTANCE].get<std::time_t>();
            andruav_servers::CAndruavFacade::getInstance().API_sendErrorMessage(std::string(), 0, ERROR_TYPE_ERROR_MODULE, NOTIFICATION_TYPE_ALERT, std::string("Module " + module_item->module_id + " has been restarted."));
        
            PLOG(plog::warning)<<"Module has been restarted: " << module_item->module_id ;
        }
        
        if (module_item->licence_status == ENUM_LICENCE::LICENSE_NOT_VERIFIED)
        {  // module has not been tested last time maybe because Auth was not ready
            checkLicenseStatus(module_item);
        }
    }

    module_item->module_last_access_time = now;

            
    
    const std::string module_class = module_item->module_class; 
    if (module_class.find(MODULE_CLASS_VIDEO)==0)
    {
        // update camera list
        std::cout  << _LOG_CONSOLE_BOLD_TEXT << "Module Found: " << _SUCCESS_CONSOLE_BOLD_TEXT_ << MODULE_CLASS_VIDEO << _INFO_CONSOLE_TEXT << "  id-" << module_id << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
        updateCameraList(module_id, msg_cmd);
        m_status.is_camera_module_connected (true);
    }
    else if ((!m_status.is_fcb_module_connected()) && (module_class.find(MODULE_CLASS_FCB)==0))
    {
        std::cout  << _LOG_CONSOLE_BOLD_TEXT << "Module Found: " << _SUCCESS_CONSOLE_BOLD_TEXT_ << MODULE_CLASS_FCB << _INFO_CONSOLE_TEXT << "  id-" << module_id << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
        CAndruavUnitMe& andruav_unit_me = CAndruavUnitMe::getInstance();
        ANDRUAV_UNIT_INFO& andruav_unit_info = andruav_unit_me.getUnitInfo();
        andruav_unit_info.use_fcb = true;
        m_status.is_fcb_module_connected (true); 
    } 
    else if ((!m_status.is_p2p_module_connected()) && (module_class.find(MODULE_CLASS_P2P)==0))
    {
        std::cout  << _LOG_CONSOLE_BOLD_TEXT << "Module Found: " << _SUCCESS_CONSOLE_BOLD_TEXT_ << MODULE_CLASS_P2P << _INFO_CONSOLE_TEXT << "  id-" << module_id << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
        m_status.is_p2p_module_connected(true);
    }
    else if ((!m_status.is_sdr_module_connected()) && (module_class.find(MODULE_CLASS_SDR)==0))
    {
        std::cout  << _LOG_CONSOLE_BOLD_TEXT << "Module Found: " << _SUCCESS_CONSOLE_BOLD_TEXT_ << MODULE_CLASS_SDR << _INFO_CONSOLE_TEXT << "  id-" << module_id << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
        m_status.is_sdr_module_connected(true);
    }
    else if ((!m_status.is_gpio_module_connected()) && (module_class.find(MODULE_CLASS_GPIO)==0))
    {
        std::cout  << _LOG_CONSOLE_BOLD_TEXT << "Module Found: " << _SUCCESS_CONSOLE_BOLD_TEXT_ << MODULE_CLASS_GPIO << _INFO_CONSOLE_TEXT << "  id-" << module_id << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
        m_status.is_gpio_module_connected(true);
    }
    

    else if ((!m_status.is_sound_module_connected()) && (module_class.find(MODULE_CLASS_SOUND)==0))
    {
        std::cout  << _LOG_CONSOLE_BOLD_TEXT << "Module Found: " << _SUCCESS_CONSOLE_BOLD_TEXT_ << MODULE_CLASS_SOUND << _INFO_CONSOLE_TEXT << "  id-" << module_id << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
        m_status.is_sound_module_connected(true);
    }

    updated |= updateUavosPermission(module_item->modules_features); 

    // reply with identification if required by module
    if (validateField(msg_cmd, JSON_INTERMODULE_RESEND, Json::value_t::boolean))
    {
        if (msg_cmd[JSON_INTERMODULE_RESEND].get<bool>() == true)
        {
            const Json &msg = createJSONID(false);
            std::string msg_dump = msg.dump();    
            forwardMessageToModule(msg_dump.c_str(), msg_dump.length() ,module_item);
        }
    }

    return updated;
}


/**
 * @brief 
 * Process messages received from module and may forward to DroneEngage Communication server.
 * @details 
 * @param full_message 
 * @param full_message_length 
 * @param ssock sender module ip & port
 */
void de::comm::CUavosModulesManager::parseIntermoduleMessage (const char * full_message, const std::size_t full_message_length, const struct sockaddr_in* ssock)
{
    if (m_exit) return ;

    Json jsonMessage;
    try
    {
        jsonMessage = Json::parse(full_message);
    }
    catch (...)
    {
        // corrupted message.
        #ifdef DEBUG
            std::cout<< _ERROR_CONSOLE_BOLD_TEXT_ << "CORRUPTED" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif
        return ;
    }

    
    const std::size_t first_string_length = strlen (full_message);

    // IMPORTANT
    // criteria: end of string part (json) is not end of the whole received data.
    // -1 is used because there is always '/0' after the string.
    // so if message is binary then you need to remobe the last character from the message
    const bool is_binary =  !(first_string_length == full_message_length-1);
    std::size_t actual_useful_size = is_binary?full_message_length-1:full_message_length;
    
    #ifdef DEBUG
    #ifdef DEBUG_MSG        
        std::cout<< jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_TYPE] << std::endl;
    #endif
    #endif

    if ((!validateField(jsonMessage, INTERMODULE_ROUTING_TYPE, Json::value_t::string))
        || (!validateField(jsonMessage, ANDRUAV_PROTOCOL_MESSAGE_TYPE, Json::value_t::number_unsigned))
        || !jsonMessage.contains(ANDRUAV_PROTOCOL_MESSAGE_CMD)
        )
    {
        #ifdef DEBUG
            std::cout<< _ERROR_CONSOLE_BOLD_TEXT_ << "BAD MESSAGE FORMAT" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif
        return ;
    }
    
    std::string target_id = std::string();
    std::string msg_routing_type = jsonMessage[INTERMODULE_ROUTING_TYPE].get<std::string>();
    
    const bool is_system = (msg_routing_type.find(CMD_COMM_SYSTEM) != std::string::npos);
    
    if ((msg_routing_type.find(CMD_COMM_GROUP) == std::string::npos)
        && (!is_system)
        && jsonMessage.contains(ANDRUAV_PROTOCOL_TARGET_ID)
        )
    {   //CMD_COMM_GROUP  does not exist and a single target id is mentioned.
        target_id =jsonMessage[ANDRUAV_PROTOCOL_TARGET_ID].get<std::string>();
    }

    // Intermodule Message
    const bool intermodule_msg = (jsonMessage[INTERMODULE_ROUTING_TYPE].get<std::string>().find(CMD_TYPE_INTERMODULE) != std::string::npos);

    const int message_type = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_TYPE].get<int>();
    const Json ms = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD];

    /*
        The logic below is as follows:

        case .... MSG ....:
            regardless of the value of [intermodule_msg] of these messages, these messages are captured by 
            case statements and handled in a way predefined my communicator module.


        The default section uses the concept of [intermodule_msg] where messages are not forwarded to server
        if it is marked intermodule_msg=true as it should be processed by other modules ONLY.
        
        the other default section forwards the message normally to andruav communication server.
    */    
    std::string module_key = "";
    
    if (jsonMessage.contains(INTERMODULE_MODULE_KEY))
    {
        module_key = jsonMessage[INTERMODULE_MODULE_KEY];
    }
                        
    switch (message_type)
    {
        case TYPE_AndruavModule_ID:
        {
            const bool updated = handleModuleRegistration (ms, ssock);
            
            if (updated == true)
            {
                andruav_servers::CAndruavFacade::getInstance().API_sendID(target_id);
            }
            
        }
        break;

        case TYPE_AndruavMessage_Mission_Item_Sequence:
        {

            // events received from other modules.
            const Json cmd = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD];


            if (validateField(cmd, "s", Json_de::value_t::string))
            {
                // string droneengage event format.
                mission::CMissionManagerBase::getInstance().getCommandsAttachedToMavlinkMission(cmd["s"].get<std::string>());
            }
        }
        break;

        case TYPE_AndruavMessage_Sync_EventFire:
        {
            /**
             * @brief Logic:
             *  This message is sent from other modules.
             * An event can be internal only and processed by comm module and other modules in the unit.
             * or can be non-internal and needs to be sent to other units.
             * 
             */

            
            // events received from other modules.
            const Json cmd = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD];


            if (validateField(cmd, "d", Json_de::value_t::string))
            {
                // string droneengage event format.
                mission::CMissionManagerBase::getInstance().fireWaitingCommands(cmd["d"].get<std::string>());
            }
            
            if (!intermodule_msg)
            {
                // broadcast to other units on the system.
                de::andruav_servers::CAndruavCommServerManager::getInstance().sendMessageToCommunicationServer (full_message, full_message_length, is_system, is_binary, target_id, message_type, ms);
            }
        }
        break;

        case TYPE_AndruavModule_RemoteExecute:
        {   // this is an inter-module message.
            processModuleRemoteExecute(ms);
        }
        break;

        case TYPE_AndruavModule_Location_Info:
        {
            /*
              IMPORTANT
              This is an inter-module message to make communicator-module aware of vehicle location.
              This message can be sent from any module who owns any information about location and motion.
            */
            
            CAndruavUnitMe& m_andruavMe = CAndruavUnitMe::getInstance();
            ANDRUAV_UNIT_LOCATION&  location_info = m_andruavMe.getUnitLocationInfo();

            location_info.latitude                      = ms["la"].get<int>();
            location_info.longitude                     = ms["ln"].get<int>();
            location_info.altitude                      = ms.contains("a")?ms["a"].get<int>():0;
            location_info.altitude_relative             = ms.contains("r")?ms["r"].get<int>():0;
            location_info.h_acc                         = ms.contains("ha")?ms["ha"].get<int>():0;
            location_info.yaw                           = ms.contains("y")?ms["y"].get<int>():0;
            location_info.last_access_time              = get_time_usec();
            location_info.is_new                        = true;
            location_info.is_valid                      = true;
        
            if (jsonMessage.contains(INTERMODULE_MODULE_KEY)!=false) // backward compatibility
            {
                processIncommingServerMessage (target_id, message_type, full_message, actual_useful_size, module_key);
            }
        }
        break;

        case TYPE_AndruavMessage_ID:
        {
            /*
                This message is always internal message sent to communicator-module CM.
                CM updates fields of the original TYPE_AndruavMessage_ID and 
                forwards a complete copy to Andruav-Server.
            */
            CAndruavUnitMe& m_andruavMe = CAndruavUnitMe::getInstance();
            ANDRUAV_UNIT_INFO&  unit_info = m_andruavMe.getUnitInfo();
            
            unit_info.vehicle_type                  = ms["VT"].get<int>();
            unit_info.flying_mode                   = ms["FM"].get<int>();
            unit_info.gps_mode                      = ms["GM"].get<int>();
            unit_info.use_fcb                       = ms["FI"].get<bool>();
            unit_info.autopilot                     = ms["AP"].get<int>();
            unit_info.armed_status                  = ms["AR"].get<int>();
            unit_info.is_flying                     = ms["FL"].get<bool>();
            unit_info.telemetry_protocol            = ms["TP"].get<int>();
            unit_info.flying_last_start_time        = ms["z"].get<long long>();
            unit_info.flying_total_duration         = ms["a"].get<long long>();
            unit_info.is_tracking_mode              = ms["b"].get<bool>();
            unit_info.manual_TX_blocked_mode        = ms["C"].get<int>();
            unit_info.is_gcs_blocked                = ms["B"].get<bool>();
            unit_info.swarm_follower_formation      = ms["n"].get<int>();
            unit_info.swarm_leader_formation        = ms["o"].get<int>();
            unit_info.swarm_leader_I_am_following   = ms["q"].get<std::string>();

            andruav_servers::CAndruavFacade::getInstance().API_sendID(std::string());
        }
        break;

        case TYPE_AndruavMessage_IMG:
        { 
                
            /**
             * @brief The message could be internal or not.
             * if it is not internal then forward it directly to server.
             * if it is INTERNAL MESSAGE then communicator module should 
             * ADD LOCATION information to image if exists.
             *
             */
            if (!intermodule_msg)
            {
                de::andruav_servers::CAndruavCommServerManager::getInstance().sendMessageToCommunicationServer (full_message, actual_useful_size, is_system, is_binary, target_id, message_type, ms);
        
                break;
            }
            
            Json msg_cmd = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD];
            
            de::andruav_servers::CAndruavCommServerManager::getInstance().sendMessageToCommunicationServer (full_message, full_message_length, is_system, is_binary, target_id, message_type, msg_cmd);
        }
        break;


        case TYPE_AndruavMessage_SWARM_MAVLINK:
        {
            // !BUG HERE WILL NOT WORK
            // forward SWARM_MAVLINK traffic to P2P by default.
            // if there is no connection then use Communication Server.
            //      Intermodule_msg is used here because p2p module may fail to forward the message 
            //      so it resends it with internal_flag=false
            de::STATUS &m_status = de::STATUS::getInstance();
            if ((intermodule_msg)&&(m_status.is_p2p_module_connected()))
            {
                processIncommingServerMessage (target_id, message_type, full_message, actual_useful_size, module_key);
                break;
            }
            else
            {
                // forward the messages normally through the server.
                de::andruav_servers::CAndruavCommServerManager::getInstance().sendMessageToCommunicationServer (full_message, full_message_length, is_system, is_binary, target_id, message_type, ms);

                // TODO: IMPORTANT: There is no gurantee that P2P is working fine.... so we need a confirmation from P2P
                // or P2P can resend SWARM_MAVLINK again and request forward to server directly.
            }
        }
        break;


        

        default:
        {
            /**
             * @brief 
             *      The default section uses the concept of [intermodule_msg] where messages are not forwarded to server
             *  if it is marked intermodule_msg=true as it should be processed by other modules only.
             * 
             */
            

            if (jsonMessage.contains(INTERMODULE_MODULE_KEY)!=false) // backward compatibility
            {
                processIncommingServerMessage (target_id, message_type, full_message, actual_useful_size, module_key);
            }

            if (!intermodule_msg)
            {   
                de::andruav_servers::CAndruavCommServerManager::getInstance().sendMessageToCommunicationServer (full_message, actual_useful_size, is_system, is_binary, target_id, message_type, ms);
            }
            else
            {
                #ifdef DDEBUG_PARSER
                    std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "ERROR:" << _TEXT_BOLD_HIGHTLITED_ << "Unhandeled internal event:" << std::to_string(message_type) <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
                #endif

            }
        }
        break;
    }
    
}

/**
 * @brief process requests from module to comm module.
 * 
 * @param ms 
 */
void de::comm::CUavosModulesManager::processModuleRemoteExecute (const Json ms)
{
    if (!validateField(ms, "C", Json::value_t::number_unsigned)) return ;
    const int cmd = ms["C"].get<int>();
    
    switch (cmd)
    {
        case TYPE_AndruavSystem_LoadTasks:
        {
            andruav_servers::CAndruavFacade::getInstance().API_loadTasksByScope(andruav_servers::ENUM_TASK_SCOPE::SCOPE_GROUP, TYPE_AndruavMessage_ExternalGeoFence);
            andruav_servers::CAndruavFacade::getInstance().API_loadTasksByScope(andruav_servers::ENUM_TASK_SCOPE::SCOPE_GROUP, TYPE_AndruavMessage_UploadWayPoints);
        }
        break;
    }
   
}


/**
 * @brief Process messages comming from Communcation Server or other modules and forward it to subscribed modules.
 * 
 * @param sender_party_id  // not used
 * @param command_type 
 * @param jsonMessage 
 * @param sender_module_key when message is forwarded from another module then it is necessary not to send message back to the sender module. e.g. messages such as TYPE_AndruavMessage_RemoteExecute
 */
void de::comm::CUavosModulesManager::processIncommingServerMessage (const std::string& sender_party_id, const int& message_type, const char * message, const std::size_t datalength, const std::string& sender_module_key)
{
    const std::lock_guard<std::mutex> lock(g_i_mutex_process);
    #ifdef DEBUG
    #ifdef DEBUG_MSG        
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: processIncommingServerMessage " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    #endif

    std::vector<std::string> &v = m_module_messages[message_type];
    for(std::vector<std::string>::iterator it = v.begin(); it != v.end(); ++it) 
    {
        #ifdef DEBUG
        #ifdef DEBUG_MSG        
        std::cout << *it << std::endl;
        #endif
        #endif
        
        auto uavos_module = m_modules_list.find(*it);
        if (uavos_module == m_modules_list.end()) 
        {
            // no module is registered for this message.
            std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Module " << *it  << " for message " << message_type << " is not available" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            
            continue;
        }
        else
        {
            
            MODULE_ITEM_TYPE * module_item = uavos_module->second.get();        
            if  (module_item->licence_status == LICENSE_VERIFIED_BAD)
            {
                andruav_servers::CAndruavFacade::getInstance().API_sendErrorMessage(std::string(), 0, ERROR_TYPE_ERROR_MODULE, NOTIFICATION_TYPE_ALERT, std::string("Module " + module_item->module_id + " is not allowed to run."));
                continue; //skip this module.
            } else if ((module_item->is_dead == false) 
                  && ((sender_module_key.empty()) 
                    // Dont send message back to its sender module [INTERMODULE_MODULE_KEY]
                    || (module_item->module_key.find(sender_module_key)==std::string::npos))
                    )
            {
                // !BUG: if sender_module_key is empty message can be sent back to sender module.
                #ifdef DEBUG
                #ifdef DEBUG_MSG        
                std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Module " << *it  << " for message " << message_type << " is Available" << _NORMAL_CONSOLE_TEXT_ << std::endl;
                #endif
                #endif 

                // clear to send
                forwardMessageToModule (message, datalength, module_item);
            }
        }
    }

    return ;
}


/**
* @brief The function is IMPORTANT it is used by DroneEngageCommunicator as a Main Module to forward messages
* to other modules.
* 
*/
void de::comm::CUavosModulesManager::forwardCommandsToModules(const int& message_type, const Json_de&  message)
{
    Json_de json_msg  = CAndruavMessage::getInstance().generateJSONMessage(CMD_COMM_INDIVIDUAL, std::string(""), std::string(""), TYPE_AndruavMessage_GPIO_ACTION, message);
        
    std::string cmd = json_msg.dump();
    std::cout << "cmd:" << cmd.c_str() << " ::: len:" << cmd.length() << std::endl;

    de::comm::CUavosModulesManager::getInstance().processIncommingServerMessage(std::string(""), TYPE_AndruavMessage_GPIO_ACTION, cmd.c_str(), cmd.length(), std::string(""));
}



/**
 * @brief forward a message from Communication Server inter-module to a module.
 * Normally this module is subscribed in this message id.
 * 
 * @param jsonMessage 
 * @param module_item 
 */
void de::comm::CUavosModulesManager::forwardMessageToModule ( const char * message, const std::size_t datalength, const MODULE_ITEM_TYPE * module_item)
{
    const std::lock_guard<std::mutex> lock(g_i_mutex_process2);
    
    #ifdef DEBUG
    #ifdef DEBUG_MSG        
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: forwardMessageToModule: " << message << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    #endif
    
    struct sockaddr_in module_address = *module_item->m_module_address.get();  
                
    cUDPClient.SendMsg(message, datalength, &module_address);

    return ;
}

/**
* @brief Check m_modules_list for dead modules that recieved no data.
* *Note: that restarted modules have the same ID not the same Key.... 
* * so restarted modules does overwrite old instances..
* 
* @return true found new dead modules.... already dead modules are not counted.
* @return false no dead modules.
*/
bool de::comm::CUavosModulesManager::handleDeadModules ()
{
    
    const std::lock_guard<std::mutex> lock(g_i_mutex);
    
    bool dead_found = false;

    const uint64_t now = get_time_usec();
    
    MODULE_ITEM_LIST::iterator it;
    
    for (it = m_modules_list.begin(); it != m_modules_list.end(); it++)
    {
        MODULE_ITEM_TYPE * module_item = it->second.get();
        const uint64_t diff =  (now - module_item->module_last_access_time);

        if (diff > MODULE_TIME_OUT)
        {
            
            if (!module_item->is_dead)
            {
                //TODO Event Module Warning
                module_item->is_dead = true;
                dead_found = true;
                if (m_status.is_online())
                {
                    std::string log_msg = std::string("Module " + module_item->module_id + " is not responding.");
                    andruav_servers::CAndruavFacade::getInstance().API_sendErrorMessage(std::string(), 0, ERROR_TYPE_ERROR_MODULE, NOTIFICATION_TYPE_EMERGENCY, log_msg);
                    PLOG(plog::error)<< log_msg ;
                }

                if (module_item->module_class.find(MODULE_CLASS_FCB)==0)
                {
                    CAndruavUnitMe& andruav_unit_me = CAndruavUnitMe::getInstance();
                    ANDRUAV_UNIT_INFO& andruav_unit_info = andruav_unit_me.getUnitInfo();
                    andruav_unit_info.use_fcb = false;
                    m_status.is_fcb_module_connected (false); //TODO: fix when offline
                }
                else if (module_item->module_class.find(MODULE_CLASS_VIDEO)==0)
                {
                    m_status.is_camera_module_connected (false); //TODO: fix when offline
                }
                else if (module_item->module_class.find(MODULE_CLASS_P2P)==0)
                {
                    m_status.is_p2p_module_connected(false);
                }
                else if (module_item->module_class.find(MODULE_CLASS_SDR)==0)
                {
                    m_status.is_sdr_module_connected(false);
                }
                else if (module_item->module_class.find(MODULE_CLASS_SOUND)==0)
                {
                    m_status.is_sound_module_connected(false);
                }
            }
        }
        else
        {
            if (module_item->is_dead)
            {
                //This should not happen as is_dead = false is done when receiving any message from a module. 
                //because you dont want running consequence operation on a dead module.
                module_item->is_dead = false;
                if (m_status.is_online())
                {
                    std::string log_msg = std::string("Module " + module_item->module_id + " is back online.");
                    andruav_servers::CAndruavFacade::getInstance().API_sendErrorMessage(std::string(), 0, ERROR_TYPE_ERROR_MODULE, NOTIFICATION_TYPE_NOTICE, log_msg);
                    PLOG(plog::warning) << log_msg ;
                }
            }
            
        }

    }
    
    return dead_found;
}

/**
 * @brief Called from @link CAndruavCommServer @endlink when connection status is updated.
 * @details all modules should be notified with status to take approbriate actions.
 * 
 * @param status 
 */
void de::comm::CUavosModulesManager::handleOnAndruavServerConnection (const int status)
{
    #ifdef DEBUG
    #ifdef DEBUG_MSG        
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: handleOnAndruavServerConnection " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    #endif

    if (m_exit) return ;
    const std::lock_guard<std::mutex> lock(g_i_mutex);
    
    MODULE_ITEM_LIST::iterator it;
    const Json &msg = createJSONID(false);
    std::string msg_dump = msg.dump();    
    
    
    for (it = m_modules_list.begin(); it != m_modules_list.end(); it++)
    {
        MODULE_ITEM_TYPE * module_item = it->second.get();

        forwardMessageToModule(msg_dump.c_str(), msg_dump.length(),module_item);
    }
}


Json de::comm::CUavosModulesManager::getModuleListAsJSON ()
{
    Json modules = Json::array();
    
    const std::lock_guard<std::mutex> lock(g_i_mutex);
    
    MODULE_ITEM_LIST::iterator it;
    
    
    for (it = m_modules_list.begin(); it != m_modules_list.end(); it++)
    {
        MODULE_ITEM_TYPE * module_item = it->second.get();

        Json json_module_entry =
        {
            // check uavos_camera_plugin
            {"v", module_item->version},
            {"i", module_item->module_id},
            {"c", module_item->module_class},
            {"t", module_item->time_stamp},
            {"d", module_item->is_dead},
        };
        modules.push_back(json_module_entry);
    }

    return modules;
}