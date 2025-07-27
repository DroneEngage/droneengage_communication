#ifndef de_modules_manager_H_
#define de_modules_manager_H_

#include <iostream>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <ctime>
#include <map>
#include <memory> 
#include <vector> 
#include <sys/socket.h>


#include "../helpers/json_nlohmann.hpp"
using Json = nlohmann::json;

#include "../global.hpp"
#include "../status.hpp"
#include "udpCommunicator.hpp"
#include "andruav_message_buffer.hpp"


#define MODULE_CLASS_COMM                       "comm"
#define MODULE_CLASS_FCB                        "fcb"
#define MODULE_CLASS_VIDEO                      "camera"
#define MODULE_CLASS_P2P                        "p2p"
#define MODULE_CLASS_SOUND                      "snd"
#define MODULE_CLASS_GPIO                       "gpio"
#define MODULE_CLASS_GENERIC                    "gen"
#define MODULE_CLASS_SDR                        "sdr"
#define MODULE_CLASS_A_RECOGNITION              "aiq" 
#define MODULE_CLASS_TRACKING                   "trk" 

#define MODULE_FEATURE_RECEIVING_TELEMETRY      "R"
#define MODULE_FEATURE_SENDING_TELEMETRY        "T"
#define MODULE_FEATURE_CAPTURE_IMAGE            "C"
#define MODULE_FEATURE_CAPTURE_VIDEO            "V"
#define MODULE_FEATURE_GPIO                     "G"
#define MODULE_FEATURE_AI_RECOGNITION           "A"
#define MODULE_FEATURE_TRACKING                 "K"
#define MODULE_FEATURE_P2P                      "P"




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
typedef struct MODULE_ITEM_TYPE_TAG
{
    std::string module_id;
    std::string module_class;
    Json modules_features = Json::array();
    std::string module_key;
    std::string hardware_serial;
    std::string version;
    int hardware_type;
    uint64_t module_last_access_time = 0;
    bool is_dead = false;
    ENUM_LICENCE licence_status = ENUM_LICENCE::LICENSE_NO_DATA;
    std::unique_ptr<struct sockaddr_in> m_module_address;
    std::time_t time_stamp = 0;
} MODULE_ITEM_TYPE;


typedef struct MODULE_CAMERA_ENTRY_TAG
{
    std::string module_id;
    Json camera_entry;      //[Active, id, ln, p, r , v]
    std::string global_index;  // id
    std::string logical_name;
    bool is_recording;
    bool is_camera_avail;
    int is_camera_streaming;
    int camera_type; // Internal & External.... legacy ... always external now.
    int actual_FPS; // latest FPS actually read from camera. valid only if recording or streaming.
    uint64_t camera_specification =0; 
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

namespace de
{
namespace comm
{
    /**
     * @brief Handles different de modules registration, updating, message forwarding and calling back.
     * 
     */
    class CUavosModulesManager : public CCallBack_UDPCommunicator
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

            CUavosModulesManager():cUDPClient(this)
            {
                m_instance_time_stamp = std::time(nullptr);
                
            }


        public:
            
            ~CUavosModulesManager (){};
           
         public:

            void defineModule (std::string module_class, 
                            std::string module_id, 
                            std::string module_key, 
                            std::string module_version, 
                            std::string party_id, 
                            std::string group_id
                        );
            
            bool init (const std::string host, int listenningPort, int chunkSize);
            void uninit();
         
            void parseIntermoduleMessage (const char * full_mesage, const std::size_t full_message_length, const struct sockaddr_in* ssock);
            Json createJSONID (const bool& reSend);
            
            void processModuleRemoteExecute (const Json ms);
            void processIncommingServerMessage (const std::string& sender_party_id, const int& message_type, const char * message, const std::size_t datalength, const std::string& sender_module_key);
            /**
             * @brief The function is IMPORTANT it is used by DroneEngageCommunicator as a Main Module to forward messages
             * to other modules.
             * 
             */
            void forwardCommandsToModules(const int& message_type, const Json_de& message);
            /**
             * @brief Get the Camera List object that defines all camera devices attached to all camera modules.
             * 
             * @return Json 
             */
            Json getCameraList() const;

            bool handleDeadModules();

            void handleOnAndruavServerConnection (const int status);
            
            Json getModuleListAsJSON();

        public:

            void setMessageOnReceive (void (*onReceive)(const char *, int len))
            {
                m_OnReceive = onReceive;
            }
        
            void onReceive (const char * message, int len, struct sockaddr_in *  sock) override;
            void (*m_OnReceive)(const char *, int len) = nullptr;
        
        private:
            void consumerThreadFunc();
            void forwardMessageToModule (const char * message, const std::size_t datalength, const MODULE_ITEM_TYPE * module_item);
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

            
            void cleanOrphanCameraEntries (const std::string& module_id, const uint64_t time_now);

            
            void checkLicenseStatus(MODULE_ITEM_TYPE * module_item);

            
            
        private:

            /**
             * @brief Identifies the type of the module.
             * Modules of the same identical functions should have the same class
             * For example multiple camera modules should have same MODULE_CLASS_VIDEO.
             * 
             */
            std::string m_module_class;

            /**
             * @brief this id identifies this particular module. This can be 
             * FCB_Main, FCB_Secondary ....etc.
             * or CAM_MOD1, CAM_MOD2 ...etc.
             * 
             */
            std::string m_module_id;

            /**
             * @brief this is a GUID key for this module. the difference between
             * m_module_id & m_module_key is that m_module_key should be unique over
             * the whole system.
             * 
             */
            std::string m_module_key;
            
            
            std::string m_module_version;

            std::string m_party_id;
            std::string m_group_id;

            std::time_t m_instance_time_stamp;
    
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

            
            de::STATUS &m_status = de::STATUS::getInstance();
            
            CUDPCommunicator cUDPClient; 

            bool m_exit = false;

            de::comm::CMessageBuffer m_buffer;
            std::thread m_consumerThread;
            bool m_running = true;
            
    };
}
}
#endif