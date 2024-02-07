#include <iostream>
#include <cstring> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <iomanip>

#include <plog/Log.h> 
#include "plog/Initializers/RollingFileInitializer.h"


#include "../helpers/colors.hpp"
#include "../helpers/json.hpp"
#include "../helpers/helpers.hpp"

#include "../messages.hpp"
#include "../comm_server/andruav_facade.hpp"
#include "andruav_comm_p2p.hpp"


using Json = nlohmann::json;




uavos::andruav_servers::CP2P::~CP2P ()
{
    
    #ifdef DEBUG
	std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: ~CP2P" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    if (m_stopped_called == false)
    {
        #ifdef DEBUG
	    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: ~CP2P" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif

        stop();
    }

    #ifdef DEBUG
	std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: ~CP2P" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    // destroy mutex
	//pthread_mutex_destroy(&m_lock);

    #ifdef DEBUG
	std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: ~CP2P" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

}

/**
 * @brief 
 * 
 * @param targetIP communication server ip
 * @param targetPort communication server port
 * @param host uavos-module listening ips default is 0.0.0.0
 * @param listenningPort uavos-module listerning port.
 */
bool uavos::andruav_servers::CP2P::init (const char * driver_ip, const int driver_port, const int channel, const char * wifi_pwd)
{
    const char * host = "0.0.0.0";
    const int listenningPort = 0;
    m_wifi_password = wifi_pwd;
    m_wifi_channel = channel;
    
    // pthread initialization
	m_thread = pthread_self(); // get pthread ID
	pthread_setschedprio(m_thread, SCHED_FIFO); // setting priority


    // Creating socket file descriptor 
    if ( (m_SocketFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        std::cout << _ERROR_CONSOLE_TEXT_ << "UDPProxy: Socket creation failed: "  << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return false;
    }

    m_ModuleAddress = new (struct sockaddr_in)();
    m_udpProxyServer = new (struct sockaddr_in)();
    memset(m_ModuleAddress, 0, sizeof(struct sockaddr_in)); 
    memset(m_udpProxyServer, 0, sizeof(struct sockaddr_in)); 
     
    // THIS MODULE (IP - PORT) 
    m_ModuleAddress->sin_family = AF_INET; 
    m_ModuleAddress->sin_port = htons(listenningPort); //listenningPort); 
    m_ModuleAddress->sin_addr.s_addr = inet_addr(host);//INADDR_ANY; 

    // UDP Proxy could have a hostname
    struct hostent * hostinfo = gethostbyname(driver_ip);
    if(!hostinfo) {
        std::cout << _ERROR_CONSOLE_TEXT_ << "UDPProxy: Cannot get info for udpProxy" << _INFO_CONSOLE_TEXT << driver_ip  << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return false;
    }
    char * target_ip = inet_ntoa(*(struct in_addr *)*hostinfo->h_addr_list);
    if (!target_ip)
    {
        std::cout << _ERROR_CONSOLE_TEXT_ << "UDPProxy: Cannot connect udp proxy " << _INFO_CONSOLE_TEXT << driver_ip  << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return false;
    }
    std::cout << _LOG_CONSOLE_TEXT_BOLD_<< "UDPProxy: Trasnlate " <<  _INFO_CONSOLE_TEXT << driver_ip << " into " <<  target_ip << _NORMAL_CONSOLE_TEXT_ << std::endl;  
    // Communication Server (IP - PORT) 
    m_udpProxyServer->sin_family = AF_INET; 
    m_udpProxyServer->sin_port = htons(driver_port); 
    m_udpProxyServer->sin_addr.s_addr = inet_addr(driver_ip); 

    // Bind the socket with the server address 
    if (bind(m_SocketFD, (const struct sockaddr *)m_ModuleAddress, sizeof(struct sockaddr_in)) > 0) 
    { 
        std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "UDPProxy: Listener  " << _ERROR_CONSOLE_TEXT_ << " BAD BIND: " << host << ":" << listenningPort << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return false ;
    } 

    std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "UDPProxy: Drone Created UDP Socket at " << _INFO_CONSOLE_TEXT << host << ":" << listenningPort << _NORMAL_CONSOLE_TEXT_ << std::endl;

    std::cout << _LOG_CONSOLE_TEXT_BOLD_<< "UDPProxy: Expected UdpProxy at " <<  _INFO_CONSOLE_TEXT << driver_ip << ":" <<  driver_port << _NORMAL_CONSOLE_TEXT_ << std::endl;  
    
    m_stopped_called = false;
    
    m_disabled = false;

    return true;
}

void uavos::andruav_servers::CP2P::start()
{
    // call directly as we are already in a thread.
    if (m_starrted == true)
        throw "Starrted called twice";

    startReceiver ();
    m_starrted = true;
}


void uavos::andruav_servers::CP2P::startReceiver ()
{
    m_threadCreateUDPSocket = std::thread {[&](){ InternalReceiverEntry(); }};
}


void uavos::andruav_servers::CP2P::stop()
{

    #ifdef DEBUG
	std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: Stop" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    m_stopped_called = true;

    if (m_SocketFD != -1)
    {
        std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "Close UDP Socket" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        //https://stackoverflow.com/questions/6389970/unblock-recvfrom-when-socket-is-closed
        shutdown(m_SocketFD, SHUT_RDWR);
    }
    
    #ifdef DEBUG
	std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: Stop" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    try
    {
        if (m_starrted) 
        {
            m_threadCreateUDPSocket.join();
            //m_threadSenderID.join();
            m_starrted = false;
        }
        delete m_ModuleAddress;
        delete m_udpProxyServer;

        #ifdef DEBUG
	    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: Stop" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif
    }
    catch(const std::exception& e)
    {
        //std::cerr << e.what() << '\n';
    }

    #ifdef DEBUG
	std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: Stop" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    

    
}

void uavos::andruav_servers::CP2P::InternalReceiverEntry()
{
    #ifdef DEBUG
	std::cout << "CP2P::InternalReceiverEntry called" << std::endl; 
    #endif
    
    struct sockaddr_in  cliaddr;
    int n;
     __socklen_t sender_address_size = sizeof (cliaddr);
    
    while (!m_stopped_called)
    {
        // TODO: you should send header ot message length and handle if total message size is larger than MAXLINE.
        n = recvfrom(m_SocketFD, (char *)buffer, MAXLINE,  
                MSG_WAITALL, ( struct sockaddr *) &cliaddr, &sender_address_size);
        
        if (n > 0) 
        {
            buffer[n]=0;
            OnMessageReceived((const char *) buffer,n);
            
            if (m_callback_udp_proxy != nullptr)
            {
                m_callback_udp_proxy->OnMessageReceived(this, (const char *) buffer,n);
            } 
        }
    }

    #ifdef DEBUG
	std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: InternalReceiverEntry EXIT" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

}


void uavos::andruav_servers::CP2P::OnMessageReceived (const char * message, int len)
{
    #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: onMessageRecieved " << message << _NORMAL_CONSOLE_TEXT_ << std::endl;
        std::cout << "P2P RX:" << message << std::endl;
    #endif

    try
    {
        Json jMsg;
        jMsg = Json::parse(message);
        
        if (!validateField(jMsg, "cmd", Json::value_t::number_unsigned))
        {
            std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "ERROR: " << _TEXT_BOLD_HIGHTLITED_ "bad p2p missing cmd number." << _INFO_CONSOLE_TEXT << "field is not defined." <<_NORMAL_CONSOLE_TEXT_ << std::endl;
            
            return ;
        }

        const int cmd_id = jMsg["cmd"].get<int>();
        switch (cmd_id)
        {
            case TYPE_P2P_MakeRestart:
            {
                std::cout << _INFO_CONSOLE_TEXT << "WARNING: P2P Device" << _ERROR_CONSOLE_BOLD_TEXT_ " Restarted." <<_NORMAL_CONSOLE_TEXT_ << std::endl;
                PLOG(plog::warning)<< "P2P Device Restarted." ;
                andruav_servers::CAndruavFacade::getInstance().API_sendErrorMessage(std::string(), 0, ERROR_TYPE_ERROR_P2P, NOTIFICATION_TYPE_CRITICAL, std::string("P2P Restarted."));

                connectAsMeshRoot(m_wifi_password, m_wifi_channel);
            }
            break ;


            case TYPE_P2P_GetMyAddress:
            {
                // example:
                // {"cmd":0,"info":{"network_type":"esp32_mesh","me":{"mac":"24:0a:c4:80:cc:d4","mac_ap":"24:0a:c4:80:cc:d5"},"connected":false}}
                const Json info = jMsg["info"];
                std::cout << "P2P RX MSG: TYPE_P2P_GetMyAddress"  << std::endl;

                ANDRUAV_UNIT_P2P_INFO&  unit_p2p_info = uavos::CAndruavUnitMe::getInstance().getUnitP2PInfo();
                unit_p2p_info.p2p_connection_type = ANDRUAV_UNIT_P2P_TYPE::unknown;

                if (info["network_type"].get<std::string>().compare("esp32_mesh")==0)
                {
                    if (!info.contains("me") ||
                        !validateField(info["me"],"mac", Json::value_t::string)     ||
                        !info["me"].contains("mac_ap")                              ||
                        !info["me"].contains("wifi_channel")                        ||
                        !validateField(info["me"],"wifi_pwd", Json::value_t::string))
                    {
                        std::cout << _ERROR_CONSOLE_TEXT_ << "P2P Mesh address: " << _ERROR_CONSOLE_BOLD_TEXT_ <<  "BAD MESSAGE FORMAT" << _NORMAL_CONSOLE_TEXT_ << std::endl;
                        return ;
                    }
                    unit_p2p_info.p2p_connection_type = ANDRUAV_UNIT_P2P_TYPE::esp32_mesh;
                    unit_p2p_info.address_1 = info["me"]["mac"];
                    unit_p2p_info.address_2 = info["me"]["mac_ap"];

                    unit_p2p_info.wifi_channel = info["me"]["wifi_channel"];
                    unit_p2p_info.wifi_password = info["me"]["wifi_pwd"];
                    
                    PLOG(plog::warning) << "P2P Device mesh address received." << unit_p2p_info.address_1 ;

                    std::cout << _INFO_CONSOLE_TEXT << "P2P Mesh address: " << _SUCCESS_CONSOLE_TEXT_ <<  unit_p2p_info.address_1 << _NORMAL_CONSOLE_TEXT_ << std::endl;

                    andruav_servers::CAndruavFacade::getInstance().API_sendErrorMessage(std::string(), 0, ERROR_TYPE_ERROR_P2P, NOTIFICATION_TYPE_NOTICE, std::string("P2P Device mesh address recieved."));

                }
            }
            break;

            case TYPE_P2P_GetParentAddress:
            {
                const Json info = jMsg["info"];

                std::cout << "P2P RX MSG: TYPE_P2P_GetParentAddress"  << std::endl;

                ANDRUAV_UNIT_P2P_INFO&  unit_p2p_info = uavos::CAndruavUnitMe::getInstance().getUnitP2PInfo();
                unit_p2p_info.parent_address = info["bssid"].get<std::string>();
                unit_p2p_info.parent_connection_status = info["connected"].get<bool>();

                PLOG(plog::warning) << "P2P Device parent received:" << unit_p2p_info.address_1 ;

                std::cout << _INFO_CONSOLE_TEXT << "P2P parent address: " << _SUCCESS_CONSOLE_TEXT_ <<  unit_p2p_info.parent_address << _NORMAL_CONSOLE_TEXT_ << std::endl;

                andruav_servers::CAndruavFacade::getInstance().API_sendErrorMessage(std::string(), 0, ERROR_TYPE_ERROR_P2P, NOTIFICATION_TYPE_NOTICE, std::string("P2P Device parent address recieved."));
                
            }   
            break;


            case TYPE_P2P_GetChildrenAddress:
            {
                
            }
            break;
        }
    }
    catch (...)
    {
        #ifdef DEBUG
        std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: onMessageRecieved " << message << _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif

    }
}

/**
 * Sends binary to Communicator
 **/
void uavos::andruav_servers::CP2P::sendMSG (const char * msg, const int length)
{
    
    try
    {
        std::cout << _INFO_CONSOLE_TEXT << "CP2P::sendMSG:::" << msg <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
    
        sendto(m_SocketFD, msg, length,  
            MSG_CONFIRM, (const struct sockaddr *) m_udpProxyServer, 
                sizeof(struct sockaddr_in));         
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

}


void uavos::andruav_servers::CP2P::restartMesh(const bool manual)
{
    Json jMsg = 
    {
        {"cmd", TYPE_P2P_MakeRestart},
        {"manual", manual}
    };

    std::string jsonStr = jMsg.dump();
    sendMSG(jsonStr.c_str(), jsonStr.length());
    return;
}

/**
 * @brief gets address of this node.
 * 
 */
void uavos::andruav_servers::CP2P::getAddress ()
{
    Json jMsg = 
    {
        {"cmd", TYPE_P2P_GetMyAddress}
    };

    std::string jsonStr = jMsg.dump();
    sendMSG(jsonStr.c_str(), jsonStr.length());
    return;
}


void uavos::andruav_servers::CP2P::connectAsMeshRoot (std::string wifi_password,  uint8_t wifi_channel)
{
    Json jMsg = 
    {
        {"cmd", TYPE_P2P_MeshCreateConnection},
        {"need_router", false},
        {"password", wifi_password},
        {"wifi_channel", wifi_channel},
        {"max_level", 1}
    };

    std::string jsonStr = jMsg.dump();
    sendMSG(jsonStr.c_str(), jsonStr.length());
    return;
}

void uavos::andruav_servers::CP2P::connectToMeshNode (const std::string mac)
{
    Json jMsg = 
    {
        {"cmd", TYPE_P2P_GetMyAddress},
        {"mac", mac}
    };

    std::string jsonStr = jMsg.dump();
    sendMSG(jsonStr.c_str(), jsonStr.length());
    return;
}