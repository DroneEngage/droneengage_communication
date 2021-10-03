#include <iostream>
#include <curl/curl.h>



#include "../helpers/colors.hpp"
#include "../helpers/helpers.hpp"

#include "../configFile.hpp"
#include "andruav_auth.hpp"


size_t WriteCallback(char *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


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
    std::string url =  "https://" + jsonConfig["auth_ip"].get<std::string>() + ":" + std::to_string(jsonConfig["auth_port"].get<int>()) +  "/agent/al/";
    //std::string url =  "https://andruav.com:19408/w/wl/";
    std::string param =  "acc=" + jsonConfig["userName"].get<std::string>()
                +  "&pwd=" + jsonConfig["accessCode"].get<std::string>() 
                + "&gr=1&app=uavos&ver=" + jsonConfig["version"].get<std::string>() 
                + "&ex=uavos&at=d";

    std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "Auth Server " << _LOG_CONSOLE_TEXT << " connection established " << _SUCCESS_CONSOLE_BOLD_TEXT_ << " successfully" << _NORMAL_CONSOLE_TEXT_ << std::endl;
#ifdef DEBUG
    std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "Auth URL: " << _TEXT_BOLD_HIGHTLITED_ << url << "?" << param << _NORMAL_CONSOLE_TEXT_ << std::endl;
#endif
       
    const int res = getAuth (url, param);


    if ((res !=CURLE_OK) || (getErrorCode() !=0))
    {
        // error 
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Andruav Authentication Failed !!" <<_NORMAL_CONSOLE_TEXT_ << std::endl;
        return false;
    }
    else
    {
        std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "Andruav Authentication Succeeded !!" <<_NORMAL_CONSOLE_TEXT_ << std::endl;
        return true;
    }
}

/**
 * @brief 
 * Performs http client connection to andruav authenticator server
 * see: https://github.com/jpbarrette/curlpp/tree/master/examples
 * @param url 
 * @param param 
 * @param response 
 * @return int 
 */
int uavos::andruav_servers::CAndruavAuthenticator::getAuth (std::string url, std::string param)
{
    CURL *easyhandle = curl_easy_init();

    std::string response;    

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
        

    curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, WriteCallback);


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
        translateResponse (response);
    }
    else
    {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Error Authentication:  (" << stringifyError(res) << ")" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    }

    return (int) res;
};



void uavos::andruav_servers::CAndruavAuthenticator::translateResponse (const std::string& response)
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
        m_is_authentication_ok = false;
        return ;
    }

    if (m_auth_error != 0)
    {
        m_is_authentication_ok = false;
        return ;
    }

    // Error Should be read before any other validation as if error some fields are not sent.
    m_auth_error = json_response["e"].get<int>();
    if (validateField (json_response, "em", Json::value_t::string))
    {
        m_auth_error_string = json_response["em"];
    }

    if (!validateField (json_response, "sid", Json::value_t::string))
    {
        m_is_authentication_ok = false;
        return ;
    }

    
    if (!validateField (json_response, "per", Json::value_t::string))
    {
        m_is_authentication_ok = false;
        return ;
    }

    if (!validateField (json_response, "cs", Json::value_t::object))
    {
        m_is_authentication_ok = false;
        return ;
    }
    
    const Json& json_comm_server = json_response["cs"];

    if (!validateField (json_comm_server, "g", Json::value_t::string))
    {
        m_is_authentication_ok = false;
        return ;
    }

    if (!validateField (json_comm_server, "h", Json::value_t::number_unsigned))
    {
        m_is_authentication_ok = false;
        return ;
    }

    if (!validateField (json_comm_server, "f", Json::value_t::string))
    {
        m_is_authentication_ok = false;
        return ;
    }

    
    m_access_code = json_response["sid"].get<std::string>();
    m_permissions = json_response["per"].get<std::string>();
    m_comm_server_ip = json_comm_server["g"].get<std::string>();
    m_comm_server_port = json_comm_server["h"].get<int>();
    m_comm_server_key = json_comm_server["f"].get<std::string>();

    

    if (m_auth_error != 0)
    {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Error Andruav Authentication:  (" << std::to_string(m_auth_error) << " - " << m_auth_error_string <<_NORMAL_CONSOLE_TEXT_ << std::endl;
    }
    
    m_is_authentication_ok = true;
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