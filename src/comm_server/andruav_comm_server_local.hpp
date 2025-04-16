#ifndef ANDRUAV_COMM_SERVER_LOCAL_H_
#define ANDRUAV_COMM_SERVER_LOCAL_H_

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <pthread.h>

#include "andruav_unit.hpp"
#include "andruav_comm_ws.hpp"
#include "../global.hpp"
#include "../de_broker/andruav_message.hpp"

#include "../helpers/json_nlohmann.hpp"
using Json_de = nlohmann::json;


namespace de
{
  
    
namespace andruav_servers
{
    class CAndruavCommServerLocal : public std::enable_shared_from_this<CAndruavCommServerLocal>, public CCallBack_WSASession
    {
        public:
            //https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
            
            static CAndruavCommServerLocal& getInstance()
            {
                static CAndruavCommServerLocal instance;

                return instance;
            };

        public:
            CAndruavCommServerLocal(CAndruavCommServerLocal const&) = delete;
            void operator=(CAndruavCommServerLocal const&) = delete;

        private:

            CAndruavCommServerLocal():m_exit(false) 
            {
                m_next_connect_time = 0;
            };
    
        public:
            
            ~CAndruavCommServerLocal (){};
            
        
        public:
            
            void start(const std::string comm_server_ip, const uint16_t comm_server_port, const std::string comm_server_key);
            void connect(const std::string comm_server_ip, const uint16_t comm_server_port, const std::string comm_server_key);
            void uninit(const bool exit);
            void turnOnOff(const bool on_off, const uint32_t duration_seconds);


            void onSocketError () override;
            void onBinaryMessageRecieved (const char * message, const std::size_t datalength) override;
            void onTextMessageRecieved (const std::string& jsonMessage) override;
                


        public:
        
            void API_pingServer();
            void API_sendSystemMessage(const int command_type, const Json_de& msg) const;
            void API_sendCMD (const std::string& target_name, const int command_type, const Json_de& msg);
            std::string API_sendCMDDummy (const std::string& target_name, const int command_type, const Json_de& msg);
            void API_sendBinaryCMD (const std::string& target_party_id, const int command_type, const char * bmsg, const uint64_t bmsg_length, const Json_de& message_cmd);
            void sendMessageToCommunicationServer (const char * full_message, const std::size_t full_message_length, const bool &is_system, const bool &is_binary, const std::string &target_id, const int msg_type, const Json_de &msg_cmd );
            int getStatus ()
            {
                return m_status;
            }

        public:
            const bool& shouldExit() const
            {
                return m_exit;
            }

            inline const u_int64_t getLastTimeAccess()
            {
                return m_lasttime_access;
            }

            
        private:
            void switchOnline();
            void switchOffline();

            
        private:
            void startWatchDogThread();

            void connectToCommServer (const std::string& server_ip, const std::string &server_port, const std::string& key, const std::string& party_id);
            
            Json_de generateJSONMessage (const std::string& message_routing, const std::string& sender_name, const std::string& target_party_id, const int messageType, const Json_de& message) const;
            Json_de generateJSONSystemMessage (const int messageType, const Json_de& message) const;
            
        private:
            de::andruav_servers::CWSAProxy& _cwsa_proxy = de::andruav_servers::CWSAProxy::getInstance();
            std::unique_ptr<de::andruav_servers::CWSASession> _cwsa_session;

            std::string m_url_param;
            std::string m_host;
            std::string m_port;
            std::string m_party_id;

            u_int8_t m_status =  SOCKET_STATUS_FRESH;

            u_int64_t m_next_connect_time,  m_lasttime_access =0;

            u_int64_t m_on_off_delay = 0;

            std::unique_ptr<std::thread> m_watch_dog;
            bool m_exit;
            CAndruavUnits& m_andruav_units = CAndruavUnits::getInstance();

            de::comm::CAndruavMessage& m_andruav_message = de::comm::CAndruavMessage::getInstance();

            
    };
}
}

#endif
