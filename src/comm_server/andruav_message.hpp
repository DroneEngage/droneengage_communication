#ifndef ANDRUAV_MESSAGE_H_
#define ANDRUAV_MESSAGE_H_



#include "../helpers/json_nlohmann.hpp"
using Json_de = nlohmann::json;

namespace de
{
  
    
namespace andruav_servers
{

class CAndruavMessage 
    {
        public:
            //https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
            
            static CAndruavMessage& getInstance()
            {
                static CAndruavMessage instance;

                return instance;
            };

        public:
            CAndruavMessage(CAndruavMessage const&) = delete;
            void operator=(CAndruavMessage const&) = delete;

        private:

            CAndruavMessage()
            {
                
            };
    
        public:
            
            ~CAndruavMessage (){};
            
        
        public:
            
            Json_de generateJSONMessage (const std::string& message_routing, const std::string& sender_name, const std::string& target_party_id, const int messageType, const Json_de& message) const;
            Json_de generateJSONSystemMessage (const int messageType, const Json_de& message) const;
            
    };
}
}


#endif
