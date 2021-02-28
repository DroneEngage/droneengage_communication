#include <iostream>
#include <curl/curl.h>

#include "./helpers/colors.hpp"
#include "./helpers/helpers.hpp"
#include "./helpers/json.hpp"
using Json = nlohmann::json;


#include "andruav_auth.hpp"
#include "andruav_comm_server.hpp"





void connectToCommServer ()
{
    uavos::andruav_servers::CAndruavAuthenticator& andruav_auth = uavos::andruav_servers::CAndruavAuthenticator::getInstance();
    uavos::andruav_servers::CAndruavCommServer& andruav_server = uavos::andruav_servers::CAndruavCommServer::getInstance();
    andruav_server.connect(andruav_auth.m_comm_server_ip, std::to_string(andruav_auth.m_comm_server_port), andruav_auth.m_comm_server_key, "oppa");
}

int main ()
{
    uavos::andruav_servers::CAndruavAuthenticator& andruav_auth = uavos::andruav_servers::CAndruavAuthenticator::getInstance();

    std::string url =  "https://192.168.1.144:19408/w/wl/";
    //std::string url =  "https://andruav.com:19408/w/wl/";
    std::string param =  "acc=mhefny@andruav.com&pwd=mhefny&gr=1&app=andruav&ver=5.0.0&ex=Andruav Web Panel&at=g";
        
    int res =0;
    //std::string response;
    res = andruav_auth.getAuth (url, param);
    if ((res !=CURLE_OK) || (andruav_auth.getErrorCode() !=0))
    {
        // error 
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Andruav Authentication Failed !!" <<_NORMAL_CONSOLE_TEXT_ << std::endl;
    }
    else
    {
        std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "Andruav Authentication Succeeded !!" <<_NORMAL_CONSOLE_TEXT_ << std::endl;
        connectToCommServer ();
    }
    

    // std::cout << "RESPONSE:" << response << std::endl;
    // Json json_response = Json::parse(response);
    // std::cout << "RESPONSE: JSON " << json_response.dump() << std::endl;
    // std::cout << "RESPONSE: JSON " << json_response["sid"].get<std::string>() << std::endl;
    
    return 0;
}