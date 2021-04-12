#ifndef ANDRUAV_COMM_SERVER_H_
#define ANDRUAV_COMM_SERVER_H_

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "andruav_comm_session.hpp"


#include "./helpers/json.hpp"
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
            };
    
        public:
            
            ~CAndruavCommServer (){};
            
        
        public:
        
            void connect (const std::string& server_ip, const std::string &server_port, const std::string& key, const std::string& party_id);
            void uninit();

            void onSocketError() override;
            //void onBinaryMessageRecieved(const std::size_t datalength) override;
            void onTextMessageRecieved(const std::string jsonMessage) override;
    
        private:
            std::shared_ptr<uavos::andruav_servers::CWSSession> _cwssession;  
            
            std::string m_url_param;
            std::string m_host;
            std::string m_port;

            u_int8_t m_status =  SOCKET_STATUS_FREASH;
    };
}
}

#endif
