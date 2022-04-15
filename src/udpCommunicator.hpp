#ifndef CUDPCLIENT_H

#define CUDPCLIENT_H

#include <thread>         // std::thread
#include <mutex>          // std::mutex, std::unique_lock


 typedef void (*ONRECEIVE_CALLBACK)(const char *, int len, struct sockaddr_in *  sock);


namespace uavos
{
namespace comm
{
class CUDPCommunicator
{

    public:
        //https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
        static CUDPCommunicator& getInstance()
        {
            static CUDPCommunicator instance;

            return instance;
        }

        CUDPCommunicator(CUDPCommunicator const&)            = delete;
        void operator=(CUDPCommunicator const&)        = delete;

    private:

        CUDPCommunicator()
        {

        }

    public:
        
        ~CUDPCommunicator ();
        void init(const char * host, int listenningPort);
        void start();
        void stop();
        void SetMessageOnReceive (ONRECEIVE_CALLBACK onReceive);
        void SendMsg(const char * message, const std::size_t datalength, struct sockaddr_in * module_address);

    protected:
        
        void startReceiver();
        void InternalReceiverEntry();
        void InternelSenderIDEntry();

        struct sockaddr_in  *m_CommunicatorModuleAddress = NULL; 
        int m_SocketFD = -1; 
        std::thread m_threadCreateUDPSocket;
        pthread_t m_thread;

        ONRECEIVE_CALLBACK m_OnReceive = NULL;
        

    protected:
        bool m_starrted = false;
        bool m_stopped_called = false;
        std::mutex m_lock;  
        
};
}
}

#endif