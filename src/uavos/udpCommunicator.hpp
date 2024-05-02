#ifndef CUDPCLIENT_H

#define CUDPCLIENT_H

#include <thread>         // std::thread
#include <mutex>          // std::mutex, std::unique_lock


 typedef void (*ONRECEIVE_CALLBACK)(const char *, int len, struct sockaddr_in *  sock);


namespace uavos
{
namespace comm
{


class CCallBack_UDPCommunicator
{
    public:
        virtual void onReceive (const char *, int len, struct sockaddr_in *  sock) {};
};


class CUDPCommunicator
{

    public:
       
       CUDPCommunicator(CCallBack_UDPCommunicator * callback)
       {
            m_callback = callback;
       }

    public:
        
        ~CUDPCommunicator ();
    
    public:
        void init(const char * host, int listenningPort);
        void start();
        void stop();
        void SendMsg(const char * message, const std::size_t datalength, struct sockaddr_in * module_address);

    protected:
        void startReceiver();
        void InternalReceiverEntry();
        void InternelSenderIDEntry();

        struct sockaddr_in  *m_CommunicatorModuleAddress = NULL; 
        int m_SocketFD = -1; 
        std::thread m_threadCreateUDPSocket;
        pthread_t m_thread;

    protected:
        bool m_starrted = false;
        bool m_stopped_called = false;
        std::mutex m_lock;  
        
        CCallBack_UDPCommunicator *m_callback = nullptr;
};
}
}

#endif