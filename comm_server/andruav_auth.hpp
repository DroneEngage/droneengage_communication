#ifndef ANDRUAV_AUTH_H_
#define ANDRUAV_AUTH_H_

#include <iostream>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>



#include "../helpers/json.hpp"

using Json = nlohmann::json;

// url commands 
#define AUTH_AGENT_LOGIN_COMMAND        "/agent/al/"
#define AUTH_AGENT_HARDWARE_COMMAND     "/agent/ah/"

// parameter names
#define AUTH_ACCOUNT_NAME_PARAMETER             "acc="
#define AUTH_SESSION_ID_PARAMETER               "sid="
#define AUTH_ACCESS_CODE_PARAMETER              "&pwd="
#define AUTH_GROUP_PARAMETER                    "&gr="
#define AUTH_APP_NAME_PARAMETER                 "&app="
#define AUTH_APP_VER_PARAMETER                  "&ver="
#define AUTH_EXTRA_PARAMETER                    "&ex="
#define AUTH_APP_VER_PARAMETER                  "&ver="
#define AUTH_HARDWARE_ID_PARAMETER              "&hi="
#define AUTH_HARDWARE_TYPE_PARAMETER            "&ht="
#define AUTH_SUB_COMMAND_PARAMETER              "&scm="
#define AUTH_ACTOR_TYPE_PARAMETER               "&at=" 

// parameter values
#define AUTH_SUB_COMMAND_VERIFY_HARDWARE_ID      "vh"
#define AUTH_ACTOR_DRONE                         "d"
#define AUTH_HARDWARE_TYPE_CPU                    1



// Reply Fields
#define AUTH_REPLY_ERROR                          "e"
#define AUTH_REPLY_ERROR_MSG                      "em"
#define AUTH_REPLY_COMM_SERVER                    "cs"
#define AUTH_REPLY_COMM_SERVER_PUBLIC_HOST        "g"
#define AUTH_REPLY_COMM_SERVER_PORT               "h"
#define AUTH_REPLY_COMM_SERVER_LOGIN_TEMP_KEY     "f"
#define AUTH_REPLY_SESSION_ID                     "sid"
#define AUTH_REPLY_PERMISSION                     "per"


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
        bool isAuthenticationOK() { return m_is_authentication_ok;}
        bool doAuthentication();
        bool doValidateHardware(const std::string hardware_id, const int hardware_type);

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

        bool getAuth (std::string url, std::string param, std::string& response);
        
        bool getAuth_doAuthentication (std::string url, std::string param);
        bool getAuth_doValidateHardware (std::string url, std::string param);
        
        std::string stringifyError (const int& error_number);
        void translateResponse_doAuthentication (const std::string& response);    
        void translateResponse_doValidateHardware (const std::string& response);    


    private:
        
        //std::string m_version;
        //std::string m_extra_info;
        
        std::string m_access_code;
        std::string m_permissions;
        std::string m_agent = "d";
        int m_auth_error =0;
        std::string m_auth_error_string;
        std::string m_hardware_error_string;
        
        bool m_is_authentication_ok = false;
};
}
}


#endif