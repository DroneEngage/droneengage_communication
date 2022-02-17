#include <iostream>
#include <curl/curl.h>


#include "../version.h"
#include "../helpers/colors.hpp"
#include "../helpers/helpers.hpp"
#include "../uavos/uavos_modules_manager.hpp"
#include "../configFile.hpp"
#include "andruav_auth.hpp"






size_t _WriteCallback(char *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


/**
 * @brief Authenticate user using username & access code.
 * @details @link getAuth_doAuthentication @endlink is called for parsing response and update class members.
 * @return true 
 * @return false 
 */
bool uavos::andruav_servers::CAndruavAuthenticator::doAuthentication()
{
    uavos::CConfigFile& cConfigFile = uavos::CConfigFile::getInstance();

    const Json& jsonConfig = cConfigFile.GetConfigJSON();
    
    if ((!validateField(jsonConfig,"auth_ip", Json::value_t::string))
     || (validateField(jsonConfig,"auth_port", Json::value_t::number_unsigned) == false)
     )
    {

        std::cout << std::to_string(validateField(jsonConfig,"auth_ip", Json::value_t::string)) << std::endl;
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "FATAL:: Missing login info in config file !!" <<_NORMAL_CONSOLE_TEXT_ << std::endl;
        exit(1);
    }

    //TODO: Move urls to auth class.
    std::string url =  "https://" + jsonConfig["auth_ip"].get<std::string>() + ":" + std::to_string(jsonConfig["auth_port"].get<int>()) +  AUTH_AGENT_LOGIN_COMMAND;
    //std::string url =  "https://andruav.com:19408/w/wl/";
    std::string param =  AUTH_ACCOUNT_NAME_PARAMETER + jsonConfig["userName"].get<std::string>()
                + AUTH_ACCESS_CODE_PARAMETER + jsonConfig["accessCode"].get<std::string>() 
                + AUTH_GROUP_PARAMETER + "1"
                + AUTH_APP_NAME_PARAMETER + "uavos"
                + AUTH_APP_VER_PARAMETER  + version_string 
                + AUTH_EXTRA_PARAMETER + "uavos"
                + AUTH_ACTOR_TYPE_PARAMETER + AUTH_ACTOR_DRONE;

    std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "Auth Server " << _SUCCESS_CONSOLE_BOLD_TEXT_ << " Connecting... " << _NORMAL_CONSOLE_TEXT_ << std::endl;
#ifdef DEBUG
    std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "Auth URL: " << _TEXT_BOLD_HIGHTLITED_ << url << "?" << param << _NORMAL_CONSOLE_TEXT_ << std::endl;
#endif
       
    const bool res = getAuth_doAuthentication (url, param);

    if (!res) //(res !=CURLE_OK) || (getErrorCode() !=0))
    {
        // error 
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Andruav Authentication Process Failed !!" <<_NORMAL_CONSOLE_TEXT_ << std::endl;
        return false;
    }
    else
    {
        if (m_is_authentication_ok)
        {
            std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "Andruav Authentication Process Done !!" <<_NORMAL_CONSOLE_TEXT_ << std::endl;
        }

        return true;
    }
}


/**
 * @brief verified hardware from database by communicating with authentication server.
 * 
 * @param hardware_id 
 * @param hardware_type 
 * @return true 
 * @return false 
 */
bool uavos::andruav_servers::CAndruavAuthenticator::doValidateHardware(const std::string hardware_id, const int hardware_type)
{
    uavos::CConfigFile& cConfigFile = uavos::CConfigFile::getInstance();

    const Json& jsonConfig = cConfigFile.GetConfigJSON();
    
    if ((!validateField(jsonConfig,"auth_ip", Json::value_t::string))
     || (validateField(jsonConfig,"auth_port", Json::value_t::number_unsigned) == false)
     )
    {

        std::cout << std::to_string(validateField(jsonConfig,"auth_ip", Json::value_t::string)) << std::endl;
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "FATAL:: Missing login info in config file !!" <<_NORMAL_CONSOLE_TEXT_ << std::endl;
        exit(1);
    }
    
    if (m_access_code.empty())
    {
        return false;
    }

    //TODO: Move urls to auth class.
    std::string url =  "https://" + jsonConfig["auth_ip"].get<std::string>() + ":" + std::to_string(jsonConfig["auth_port"].get<int>()) +  AUTH_AGENT_HARDWARE_COMMAND;
    //std::string url =  "https://andruav.com:19408/w/wl/";
    std::string param =  AUTH_SESSION_ID_PARAMETER + m_access_code
                + AUTH_HARDWARE_ID_PARAMETER + hardware_id
                + AUTH_HARDWARE_TYPE_PARAMETER + std::to_string(hardware_type)
                + AUTH_SUB_COMMAND_PARAMETER + AUTH_SUB_COMMAND_VERIFY_HARDWARE_ID;

    std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "Auth Server " << _LOG_CONSOLE_TEXT << " connection established " << _SUCCESS_CONSOLE_BOLD_TEXT_ << " successfully" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #ifdef DEBUG
        std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "HARDWARE URL: " << _TEXT_BOLD_HIGHTLITED_ << url << "?" << param << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    const int res = getAuth_doValidateHardware (url, param);


    if (!res) //(res !=CURLE_OK) || (getErrorCode() !=0))
    {
        // error 
        #ifdef DEBUG
            std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "Hardware Verification Failed !!" <<_NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif

        return false;
    }
    else
    {
        #ifdef DEBUG
            std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "Hardware Verification Succeeded !!" <<_NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif

        return true;
    }
}


/**
 * @brief Performs http client connection to andruav authenticator server
 * @see: https://github.com/jpbarrette/curlpp/tree/master/examples
 * @param url 
 * @param param 
 * @param response 
 * @return bool 
 */
bool uavos::andruav_servers::CAndruavAuthenticator::getAuth (std::string url, std::string param, std::string& response)
{
    CURL *easyhandle = curl_easy_init();

 
    if(!easyhandle) return CURLE_FAILED_INIT;
    /* Set the default value: strict certificate check please */
    curl_easy_setopt(easyhandle, CURLOPT_URL, url.c_str());

    curl_easy_setopt(easyhandle, CURLOPT_DEFAULT_PROTOCOL, "https");

    /* Now specify the POST data */ 
    curl_easy_setopt(easyhandle, CURLOPT_POSTFIELDS, param.c_str());
        
    #ifdef DEBUG
        long verify = false;
    #else
        long verify = true;
    #endif

    curl_easy_setopt(easyhandle, CURLOPT_SSL_VERIFYPEER, verify);
    curl_easy_setopt(easyhandle, CURLOPT_SSL_VERIFYHOST, verify);
        

    curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, _WriteCallback);


    //std::string readBuffer;
    curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &response);

    /* Now run off and do what you've been told! */ 
    CURLcode res = curl_easy_perform(easyhandle); /* post away! */

        
    /* always cleanup */ 
    curl_easy_cleanup(easyhandle);
    
    /* free headers */ 
    //curl_slist_free_all(headers);
    curl_global_cleanup();

    if (res == CURLE_OK)
    {
        return true;
    }
    else
    {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Error Authentication:  (" << stringifyError(res) << ")" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    }

    return false;
}

/**
 * @brief recieve and parse response
 * @details called by @link doAuthentication @endlink and it calls @link translateResponse_doAuthentication @endlink
 * @param url 
 * @param param 
 * @return true 
 * @return false 
 */
bool uavos::andruav_servers::CAndruavAuthenticator::getAuth_doAuthentication (std::string url, std::string param)
{
    std::string response;
    if (getAuth (url, param, response))
    {
        translateResponse_doAuthentication (response);
        return true;
    }

    return false;
}


bool uavos::andruav_servers::CAndruavAuthenticator::getAuth_doValidateHardware (std::string url, std::string param)
{
    std::string response;
    if (getAuth (url, param, response))
    {
        return translateResponse_doValidateHardware (response);
    }

    return false;
}

        

/**
 * @brief parse response of @link doAuthentication @endlink
 * 
 * @param response 
 */
void uavos::andruav_servers::CAndruavAuthenticator::translateResponse_doAuthentication (const std::string& response)
{
    
    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: Response: " << response << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    m_is_authentication_ok = false;

    const Json& json_response = Json::parse(response);
    
    m_auth_error = 0; //reset error;

    if (validateField (json_response, "e", Json::value_t::number_unsigned) == false)
    {   
        m_auth_error = -1;
        m_auth_error_string = "BAD XML";
        return ;
    }

   
    // Error Should be read before any other validation as if error some fields are not sent.
    m_auth_error = json_response[AUTH_REPLY_ERROR].get<int>();
    
    if (validateField (json_response, AUTH_REPLY_ERROR_MSG, Json::value_t::string))
    {
        m_auth_error_string = json_response[AUTH_REPLY_ERROR_MSG];
    }


    if (m_auth_error != 0)
    {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Error Andruav Authentication:  (" << std::to_string(m_auth_error) << " - " << m_auth_error_string <<_NORMAL_CONSOLE_TEXT_ << std::endl;
        return ;
    }


    if (!validateField (json_response, "sid", Json::value_t::string))
    {
        return ;
    }

    
    if (!validateField (json_response, "per", Json::value_t::string))
    {
        return ;
    }

    if (!validateField (json_response, AUTH_REPLY_COMM_SERVER, Json::value_t::object))
    {
        return ;
    }
    
    const Json& json_comm_server = json_response[AUTH_REPLY_COMM_SERVER];

    if (!validateField (json_comm_server, AUTH_REPLY_COMM_SERVER_PUBLIC_HOST, Json::value_t::string))
    {
        return ;
    }

    if (!validateField (json_comm_server, AUTH_REPLY_COMM_SERVER_PORT, Json::value_t::number_unsigned))
    {
        return ;
    }

    if (!validateField (json_comm_server, AUTH_REPLY_COMM_SERVER_LOGIN_TEMP_KEY, Json::value_t::string))
    {
        return ;
    }

    
    m_access_code = json_response[AUTH_REPLY_SESSION_ID].get<std::string>();
    m_permissions = json_response[AUTH_REPLY_PERMISSION].get<std::string>();
    m_comm_server_ip = json_comm_server[AUTH_REPLY_COMM_SERVER_PUBLIC_HOST].get<std::string>();
    m_comm_server_port = json_comm_server[AUTH_REPLY_COMM_SERVER_PORT].get<int>();
    m_comm_server_key = json_comm_server[AUTH_REPLY_COMM_SERVER_LOGIN_TEMP_KEY].get<std::string>();

    
    m_is_authentication_ok = true;
}



bool uavos::andruav_servers::CAndruavAuthenticator::translateResponse_doValidateHardware (const std::string& response)
{
    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: Response: " << response << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    
    const Json& json_response = Json::parse(response);
    
    m_auth_error = 0; //reset error;

    if (validateField (json_response, "e", Json::value_t::number_unsigned) == false)
    {   
        m_auth_error = -1;
        m_auth_error_string = "BAD XML";
        return false;
    }

     // Error Should be read before any other validation as if error some fields are not sent.
    m_auth_error = json_response[AUTH_REPLY_ERROR].get<int>();
    
    if (validateField (json_response, AUTH_REPLY_ERROR_MSG, Json::value_t::string))
    {
        m_hardware_error_string = json_response[AUTH_REPLY_ERROR_MSG];
    }

    if (m_auth_error != 0)
    {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Error Hardware Authentication:  (" << std::to_string(m_auth_error) << " - " << m_hardware_error_string <<_NORMAL_CONSOLE_TEXT_ << std::endl;
        return false;
    }

    return true;
}


/**
 * @brief 
 * Converts error number of enum CURLcode into readable string
 * 
 * @param error_number 
 * @return std::string 
 */
std::string uavos::andruav_servers::CAndruavAuthenticator::stringifyError (const int& error_number)
{
    return (std::string(curl_easy_strerror((CURLcode)error_number)));
}


void uavos::andruav_servers::CAndruavAuthenticator::uninit()
{
    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: uninit " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}