#ifndef ANDRUAV_COMM_SERVER_H_
#define ANDRUAV_COMM_SERVER_H_

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <pthread.h>

#include "andruav_unit.hpp"
#include "andruav_comm_session.hpp"


#include "../helpers/json.hpp"
using Json = nlohmann::json;


// SOCKET STATUS
#define SOCKET_STATUS_FREASH 			1   // socket is new
#define SOCKET_STATUS_CONNECTING    	2	// connecting to WS
#define SOCKET_STATUS_DISCONNECTING 	3   // disconnecting from WS
#define SOCKET_STATUS_DISCONNECTED 		4   // disconnected  from WS
#define SOCKET_STATUS_CONNECTED 		5   // connected to WS
#define SOCKET_STATUS_REGISTERED 		6   // connected and executed AddMe
#define SOCKET_STATUS_UNREGISTERED 		7   // connected but not registred
#define SOCKET_STATUS_ERROR 		    8   // Error

namespace uavos
{
  
    
namespace andruav_servers
{

    void *startWatchDogThread(void *args);
    void *startWatchDogThread2(void *args);


    class CAndruavCommServer : public std::enable_shared_from_this<CAndruavCommServer>, public CCallBack_WSSession
    {
        public:
            //https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
            
            static CAndruavCommServer& getInstance()
            {
                static CAndruavCommServer instance;

                return instance;
            };

        public:
            CAndruavCommServer(CAndruavCommServer const&) = delete;
            void operator=(CAndruavCommServer const&) = delete;

        private:

            CAndruavCommServer() 
            {
                m_next_connect_time = 0;
            };
    
        public:
            
            ~CAndruavCommServer (){};
            
        
        public:
            
            void start();
            void connect();
            void uninit();


            void onSocketError () override;
            void onBinaryMessageRecieved (const char * message, const std::size_t datalength) override;
            void onTextMessageRecieved (const std::string& jsonMessage) override;
                


        public:
        
            void API_pingServer();
            void API_sendSystemMessage(const int command_type, const Json& msg) const;
            void API_sendCMD (const std::string& target_party_id, const int command_type, const Json& msg);
            void API_sendBinaryCMD (const std::string& target_party_id, const int command_type, const char * bmsg, const int bmsg_length, const Json& message_cmd);

            int getStatus ()
            {
                return m_status;
            }

        public:
            const bool& shouldExit()
            {
                return m_exit;
            }

            inline const u_int64_t getLastTimeAccess()
            {
                return m_lasttime_access;
            }

            
        private:

            void connectToCommServer (const std::string& server_ip, const std::string &server_port, const std::string& key, const std::string& party_id);
            void parseCommand (const std::string& sender_party_id, const int& command_type, const Json& jsonMessage);
            void parseRemoteExecuteCommand (const std::string& sender_party_id, const Json& jsonMessage);
            
            Json generateJSONMessage (const std::string& message_routing, const std::string& sender_name, const std::string& target_party_id, const int messageType, const Json& message) const;
            Json generateJSONSystemMessage (const int messageType, const Json& message) const;
            
        private:
            std::shared_ptr<uavos::andruav_servers::CWSSession> _cwssession;  
            
            std::string m_url_param;
            std::string m_host;
            std::string m_port;
            std::string m_party_id;

            u_int8_t m_status =  SOCKET_STATUS_FREASH;

            u_int64_t m_next_connect_time,  m_lasttime_access =0;

            pthread_t m_watch_dog;
            pthread_t m_watch_dog2;
            bool m_first = true;
            bool m_exit = false;
            CAndruavUnits& m_andruav_units = CAndruavUnits::getInstance();
    };
}
}

#endif
