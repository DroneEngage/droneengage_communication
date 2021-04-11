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

namespace uavos
{
namespace andruav_servers
{
    class CAndruavCommServer : public std::enable_shared_from_this<CAndruavCommServer>
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


        private:
            std::shared_ptr<uavos::andruav_servers::CWSSession> _cwssession;  
    };
}
}

#endif
