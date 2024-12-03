#ifndef ANDRUAV_PARSER_H_
#define ANDRUAV_PARSER_H_

#include <cstdlib>
#include <string>

#include "andruav_unit.hpp"

#include "../helpers/json_nlohmann.hpp"
using Json_de = nlohmann::json;


namespace de
{

namespace andruav_servers 
{

    class CAndruavParser
    {
        public:
            //https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
            
            static CAndruavParser& getInstance()
            {
                static CAndruavParser instance;

                return instance;
            };

        public:
            CAndruavParser(CAndruavParser const&) = delete;
            void operator=(CAndruavParser const&) = delete;

        private:

            CAndruavParser() 
            {
            };
    
        public:
            
            ~CAndruavParser (){};
            
        public:
            void parseCommand (const std::string& sender_party_id, const int& command_type, const Json_de& jsonMessage);
            void parseRemoteExecuteCommand (const std::string& sender_party_id, const Json_de& jsonMessage);
            

        private:
            
            CAndruavUnits& m_andruav_units = CAndruavUnits::getInstance();

    };
}
}



#endif
