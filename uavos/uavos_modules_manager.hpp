#ifndef UAVOS_MODULES_MANAGER_H_
#define UAVOS_MODULES_MANAGER_H_


#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#include <map>
#include <memory> 
#include <vector> 
#include <sys/socket.h>

#include "../helpers/json.hpp"
using Json = nlohmann::json;

#include "../global.hpp"

// 5 seconds
#define MODULE_TIME_OUT  5000000

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
    uint64_t module_last_access_time = 0;
    bool is_dead = false;
    std::unique_ptr<struct sockaddr_in> m_module_address;
    
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
typedef std::map <std::string, std::shared_ptr<MODULE_ITEM_TYPE>> MODULE_ITEM_LIST;


typedef std::map <std::string, std::unique_ptr<std::map 
                              <std::string,std::unique_ptr
                              <MODULE_CAMERA_ENTRY>>>> MODULE_CAMERA_LIST;

namespace uavos
{
    /**
     * @brief 
     * Handles different uavos modules registration, updating, message forwarding and calling back.
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
           
            void parseIntermoduleMessage (Json& jsonMessage, const struct sockaddr_in* ssock, bool& forward);
            Json createJSONID (const bool& reSend);
            
            void processIncommingServerMessage (const std::string& sender_party_id, const int& command_type, const Json& jsonMessage);

            void forwardMessageToModule (const Json& jsonMessage, const MODULE_ITEM_TYPE * module_item);
            
            /**
             * @brief Get the Camera List object that defines all camera devices attached to all camera modules.
             * 
             * @return Json 
             */
            Json getCameraList();

            /**
             * @brief Check m_modules_list for dead modules that recieved no data.
             * *Note: that restarted modules have the same ID not the same Key.... 
             * * so restarted modules does overwrite old instances..
             * 
             * @return true found new dead modules.... already dead modules are not counted.
             * @return false no dead modules.
             */
            bool HandleDeadModules();

        private:

            /**
             * @brief handle TYPE_AndruavModule_ID messages.
             * Add/Update module definitions.
             * @param msg_cmd 
             * @param ssock 
             * 
             * @return true module has been added.
             * @return false no new modules.
             */
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

            
            /**
             * @brief update UAVOS vehicle permission based on module permissions.
             * ex: "d" : [ "C", "V" ]
             * @param module_permissions 
             * 
             * @return true if permission updated
             * @return false if permissions the samme
             */
            bool updateUavosPermission (const Json& module_permissions);


            /**
             * @brief Update camera list.
             * Adding available camera devices exists in different camera modules.
             * 
             * RX MSG: {
                "ms" : {
                    "a" : "HorusEye1",
                    "b" : "camera",
                    "c" : [ 1005, 1041, 1021 ],
                    "d" : [ "C", "V" ],
                    "e" : "E289FEE7-FDAD-44EF-A257-C9A36DDD6BE7",
                    "m" : [
                        {
                            "active" : 0,
                            "id" : "G59d8d78965966a1a449b44b1",
                            "ln" : "Droidcam#0",
                            "p" : 2,
                            "r" : false,
                            "v" : true
                        },
                        {
                            "active" : 0,
                            "id" : "G207ac06d13bf7f2756f2fc51",
                            "ln" : "Dummy video device (0x0000)#1",
                            "p" : 2,
                            "r" : false,
                            "v" : true
                        },
                        {
                            "active" : 0,
                            "id" : "G69058c165ac352104cef76d9",
                            "ln" : "Dummy video device (0x0001)#2",
                            "p" : 2,
                            "r" : false,
                            "v" : true
                        },
                        {
                            "active" : 0,
                            "id" : "G65a44b9276d1e51e59658bc",
                            "ln" : "Dummy video device (0x0002)#3",
                            "p" : 2,
                            "r" : false,
                            "v" : true
                        }
                    ],
                    "z" : false
                },
                "mt" : 9100,
                "ty" : "uv"
                }
             * @param msg_cmd 
             */
            void updateCameraList(const std::string& module_id, const Json& msg_cmd);

            /**
             * @brief Camera module should send a complete list of camera devices.
             * Any missing camera is one disappeared most likely the module restarted 
             * and generated new camera device ids
             * 
             * @param module_id 
             */
            void cleanOrphanCameraEntries (const std::string& module_id, const uint64_t& time_now);

        private:

            /**
             * @brief 
             * list of registered modules mapped by module id
             * 
             */
            MODULE_ITEM_LIST  m_modules_list ;

            
            /**
             * @brief 
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