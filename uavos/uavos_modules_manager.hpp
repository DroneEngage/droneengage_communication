#ifndef UAVOS_MODULES_MANAGER_H_
#define UAVOS_MODULES_MANAGER_H_


#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <ctime>
#include <map>
#include <memory> 
#include <vector> 
#include <sys/socket.h>


#include "../helpers/json.hpp"
using Json = nlohmann::json;

#include "../global.hpp"

// 5 seconds
#define MODULE_TIME_OUT  5000000

enum ENUM_LICENCE 
{
    // licence exists and verified
    LICENSE_VERIFIED_OK     = 0,
    // license exixts and rejected
    LICENSE_VERIFIED_BAD    = 1,
    // license exists but has not been verified yet
    LICENSE_NOT_VERIFIED    = 2,
    // no license for this module
    LICENSE_NO_DATA         = 3
};

/**
 * @brief Structure that defines Uavos modules.
 * 
 */
typedef struct 
{
    std::string module_id;
    std::string module_class;
    Json modules_features = Json::array();
    std::string module_key;
    std::string hardware_serial;
    int hardware_type;
    uint64_t module_last_access_time = 0;
    bool is_dead = false;
    ENUM_LICENCE licence_status = ENUM_LICENCE::LICENSE_NO_DATA;
    std::unique_ptr<struct sockaddr_in> m_module_address;
    std::time_t time_stamp = 0;
} MODULE_ITEM_TYPE;


typedef struct 
{
    std::string module_id;
    Json camera_entry;      //[Active, id, ln, p, r , v]
    std::string global_index;  // id
    std::string logical_name;
    bool is_recording;
    int is_camera_avail;
    int is_camera_streaming;
    int camera_type; // Internal & External.... legacy ... always external now.
    uint64_t module_last_access_time = 0;
    bool updates;

} MODULE_CAMERA_ENTRY;

/**
 * @brief 
 * 
 */
typedef std::map <std::string, std::unique_ptr<MODULE_ITEM_TYPE>> MODULE_ITEM_LIST;


typedef std::map <std::string, std::unique_ptr<std::map 
                              <std::string,std::unique_ptr
                              <MODULE_CAMERA_ENTRY>>>> MODULE_CAMERA_LIST;

namespace uavos
{
    /**
     * @brief Handles different uavos modules registration, updating, message forwarding and calling back.
     * 
     */
    class CUavosModulesManager
    {
        public:
            //https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
            static CUavosModulesManager& getInstance()
            {
                static CUavosModulesManager instance;

                return instance;
            }

            CUavosModulesManager(CUavosModulesManager const&)           = delete;
            void operator=(CUavosModulesManager const&)                 = delete;

        private:

            CUavosModulesManager() {};


        public:
            
            ~CUavosModulesManager ();
           
            void parseIntermoduleMessage (const char * full_mesage, const std::size_t full_message_length, const struct sockaddr_in* ssock);
            Json createJSONID (const bool& reSend);
            
            void processIncommingServerMessage (const std::string& sender_party_id, const int& command_type, const char * message, const std::size_t datalength);

            void forwardMessageToModule (const char * message, const std::size_t datalength, const MODULE_ITEM_TYPE * module_item);
            
            /**
             * @brief Get the Camera List object that defines all camera devices attached to all camera modules.
             * 
             * @return Json 
             */
            Json getCameraList();

            bool handleDeadModules();

            void handleOnAndruavServerConnection (const int status);
            
        private:

            bool handleModuleRegistration (const Json& msg_cmd, const struct sockaddr_in* ssock);

            /**
             * @brief called by handleModuleRegistration to update subscribed messages for a module.
             * 
             * @param module_id 
             * @param message_array 
             * 
             * @return true module has been added.
             * @return false no new modules.
             */
            bool updateModuleSubscribedMessages (const std::string& module_id, const Json& message_array);

            
            bool updateUavosPermission (const Json& module_permissions);


            
            void updateCameraList(const std::string& module_id, const Json& msg_cmd);

            
            void cleanOrphanCameraEntries (const std::string& module_id, const uint64_t& time_now);

            
            void checkLicenseStatus(MODULE_ITEM_TYPE * module_item);

        private:

            /**
             * @brief 
             * list of registered modules mapped by module id
             * 
             */
            MODULE_ITEM_LIST  m_modules_list ;

            
            /**
             * @brief map (message number, List of subscribed modules)
             * @details 
             * callback list mapped by message ids.
             * when this message is received from andruav-server it should be forwarded to 
             * modules listed in vector of this message id.
             * * You need to use @param m_modules_list to get ip & port.
             */
            std::map <int, std::vector<std::string>> m_module_messages;


            MODULE_CAMERA_LIST m_camera_list;

            
    
            
    };
}

#endif