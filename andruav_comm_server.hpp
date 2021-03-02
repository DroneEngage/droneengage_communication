#ifndef ANDRUAV_COMM_SERVER_H_
#define ANDRUAV_COMM_SERVER_H_

#include <iostream>

#include "./helpers/json.hpp"
using Json = nlohmann::json;

namespace uavos
{
namespace andruav_servers
{
    class CAndruavCommServer
    {
        public:
            //https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
            
            static CAndruavCommServer& getInstance()
            {
                static CAndruavCommServer instance;

                return instance;
            };

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
    };
}
}

#endif
