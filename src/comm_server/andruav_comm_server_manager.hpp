#ifndef ANDRUAV_COMM_SERVER_MANAGER_H_
#define ANDRUAV_COMM_SERVER_MANAGER_H_

#include "andruav_comm_server_local.hpp"
#include "andruav_comm_server.hpp"

#include "../helpers/json_nlohmann.hpp"
using Json_de = nlohmann::json;


namespace de
{
  
    
namespace andruav_servers
{
    class CAndruavCommServerManager
    {
        public:
            //https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
            
            static CAndruavCommServerManager& getInstance()
            {
                static CAndruavCommServerManager instance;

                return instance;
            };

        public:
            CAndruavCommServerManager(CAndruavCommServerManager const&) = delete;
            void operator=(CAndruavCommServerManager const&) = delete;

        private:

            CAndruavCommServerManager()
            {
               
            };
    
        public:
            
            ~CAndruavCommServerManager (){};
            
        public:
            
            void sendMessageToCommunicationServer (const char * full_message, const std::size_t full_message_length, const bool &is_system, const bool &is_binary, const std::string &target_id, const int msg_type, const Json_de &msg_cmd );
            bool isOnline();
            void turnOnOff(const bool on_off, const uint32_t duration_seconds);
            void uninit(bool exit_mode);
            void API_sendCMD (const std::string& target_party_id, const int command_type, const Json_de& msg) const;
            const std::string API_sendCMDDummy (const std::string& target_party_id, const int command_type, const Json_de& msg) const;

    };
}
}

#endif