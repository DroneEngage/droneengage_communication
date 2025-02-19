#ifndef CUDPCLIENT_H

#define CUDPCLIENT_H

#include <thread>         // std::thread
#include <mutex>          // std::mutex, std::unique_lock


#define MAX_UDP_DATABUS_PACKET_SIZE 0xffff
#define DEFAULT_UDP_DATABUS_PACKET_SIZE 8192

typedef void (*ONRECEIVE_CALLBACK)(const char *, int len, struct sockaddr_in *  sock);

#ifndef MAXLINE
#define MAXLINE 0xffff 
#endif

namespace de
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
        void init(const char * host, int listenningPort, int chunkSize);
        void start();
        void stop();
        void SendMsg(const char * message, const std::size_t datalength, struct sockaddr_in * module_address);

    protected:
        void startReceiver();
        void InternalReceiverEntry();
        void InternelSenderIDEntry();

        int m_SocketFD = -1; 
        std::thread m_threadCreateUDPSocket;
        pthread_t m_thread;

    protected:
        bool m_starrted = false;
        bool m_stopped_called = false;
        std::mutex m_lock;  
        
        CCallBack_UDPCommunicator *m_callback = nullptr;
        std::unique_ptr<struct sockaddr_in> m_CommunicatorModuleAddress = std::make_unique<struct sockaddr_in>();
        int m_chunkSize;

        char m_buffer[MAXLINE]; 
};
}
}

#endif