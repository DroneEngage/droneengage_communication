#ifndef ANDRUAV_AUTH_H_
#define ANDRUAV_AUTH_H_

#include <iostream>

#include "./helpers/json.hpp"
using Json = nlohmann::json;

namespace uavos
{
namespace andruav_servers
{
class CAndruavAuthenticator
{

    public:
        //https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
        static CAndruavAuthenticator& getInstance()
        {
            static CAndruavAuthenticator instance;

            return instance;
        };

        CAndruavAuthenticator(CAndruavAuthenticator const&)           = delete;
        void operator=(CAndruavAuthenticator const&)        = delete;

        private:

            CAndruavAuthenticator() 
            {
            };
    
    
    public:
        
        ~CAndruavAuthenticator (){};
        
    public:
        
        int getAuth (std::string url, std::string param);
        void uninit();
        
        const int& getErrorCode()
        {
            return m_auth_error;
        }

        const std::string& getErrorString()
        {
            return m_auth_error_string;
        }
    


    public:
        
        std::string m_comm_server_ip;
        int         m_comm_server_port;
        std::string m_comm_server_key;
        

    private:

        std::string stringifyError (const int& error_number);
        void translateResponse (const std::string& response);    


    private:
        
        //std::string m_version;
        //std::string m_extra_info;
        
        std::string m_access_code;
        std::string m_permissions;
        std::string m_agent = "d";
        int m_auth_error =0;
        std::string m_auth_error_string;

        bool m_is_authentication_ok = false;
};
}
}


#endif