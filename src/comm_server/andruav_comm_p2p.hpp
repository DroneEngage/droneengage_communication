#ifndef CUDPROXY_H

#define CUDPROXY_H

#include <thread>         // std::thread
#include <mutex>          // std::mutex, std::unique_lock


#include "andruav_unit.hpp"

#ifndef MAXLINE
#define MAXLINE 65507 
#endif

#define MODULE_P2P_TIME_OUT  5000000

#define TYPE_P2P_GetMyAddress           0
#define TYPE_P2P_GetChildrenAddress     1
#define TYPE_P2P_GetParentAddress       2
#define TYPE_P2P_SendMessageToNode      3
#define TYPE_P2P_IncommingMessage       6
#define TYPE_P2P_MakeRestart            7
#define TYPE_P2P_ConnectToNodeByMac     11
#define TYPE_P2P_ScanMesh               12
#define TYPE_P2P_MeshScanResult         13
#define TYPE_P2P_MeshCreateConnection   15




namespace uavos
{
namespace andruav_servers
{

class CP2P;
class CCallBack_UdpProxy
{
    public:

        virtual void OnMessageReceived (const uavos::andruav_servers::CP2P * udp_proxy, const char * message, int len) {};
        virtual void OnConnected (const bool& connected) {};
};

/**
 * @brief This class handles P2P cimmunication between drones.
 * P2P here refers to a comunication takes place away from andruav_server.
 * It can be a direct P2P or a mesh network ...etc.
 * 
 */
class CP2P
{

   public:

        static CP2P& getInstance()
            {
                static CP2P instance;

                return instance;
            }

            CP2P(CP2P const&)                           = delete;
            void operator=(CP2P const&)                 = delete;

        private:

            CP2P() {};


        public:
            
            ~CP2P ();
        
        
    public:
        
        bool init(const char * driver_ip, const int driver_port, const int channel, const char * wifi_pwd);
        void setCallback (CCallBack_UdpProxy * callback_udp_proxy)
        {
            m_callback_udp_proxy = callback_udp_proxy;
        }
        void start();
        void stop();

        bool isUpdated();
        bool isStarted() const { return m_starrted;}
        bool isDisabled() const { return m_disabled;}
        void disable() {m_disabled = true;}
        
        
    public:

        void restartMesh(const bool manual) const ;
        void getAddress();
        void connectAsMeshRoot (std::string wifi_password, uint8_t wifi_channel) const;
        void connectToMeshNode (const std::string mac) const;
        void sendMessageToMeshNode(const std::string mac, const char * bmsg, const int bmsg_length) const;

    public:
        bool processForwardSwarmMessage(const std::string& target_id, const char * bmsg, const int bmsg_length) const;
        

    protected:
        void sendMSG(const char * msg, const int length) const ;
        void startReceiver();

        void InternalReceiverEntry();

        void OnMessageReceived (const char * message, int len);


        struct sockaddr_in  *m_udpProxyServer = nullptr; 
        struct sockaddr_in  *m_ModuleAddress = nullptr; 
        int m_SocketFD = -1; 
        std::thread m_threadCreateUDPSocket;
        pthread_t m_thread;

        std::string m_JsonID;
        void (*m_OnReceive)(const char *, int len) = nullptr;

    protected:
        bool m_starrted = false;
        bool m_stopped_called = false;
        bool m_disabled = false;
        bool m_updated = false;
        std::mutex m_lock;  

        char buffer[MAXLINE]; 

        CCallBack_UdpProxy * m_callback_udp_proxy = nullptr;

        std::string m_wifi_password;
        int m_wifi_channel;
        
        uint64_t m_last_active_time = 0;
    private:
    
        CAndruavUnits& m_andruav_units = CAndruavUnits::getInstance();

};
}
}
#endif