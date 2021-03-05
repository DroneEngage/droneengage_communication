#ifndef UAVOS_MODULES_MANAGER_H_
#define UAVOS_MODULES_MANAGER_H_


#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#include <map>
#include <memory> 
#include <vector> 
#include <sys/socket.h>

#include "./helpers/json.hpp"
using Json = nlohmann::json;

#include "global.hpp"

// 5 seconds
#define Module_TIME_OUT  5000000

/**
 * @brief 
 * Structure that defines Uavos modules.
 */
typedef struct 
{
    std::string module_id;
    std::string module_class;
    Json modules_features = Json::array();
    std::string module_key;
    uint64_t module_last_access_time = 0;

    std::unique_ptr<struct sockaddr_in> m_module_address;
    
} MODULE_ITEM_TYPE;


typedef std::map <std::string, std::shared_ptr<MODULE_ITEM_TYPE>> MODULE_ITEM_LIST;

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
           
            void parseIntermoduleMessage (Json& jsonMessage, struct sockaddr_in * ssock, bool& forward);
            Json createJSONID (const bool& reSend);
            
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

            
            
    };
}

#endif