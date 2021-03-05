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
        void init(const char * targetIP, int broadcatsPort, const char * host, int listenningPort);
        void start();
        void stop();
        void SetMessageOnReceive (ONRECEIVE_CALLBACK onReceive);
        void SendJMSG(const std::string& jmsg, struct sockaddr_in * module_address);

    protected:
        // This static function only needed once
        // it sends ID to communicator. 
        // you need to create UDP with communicator first.
        //// static void * InternalSenderIDThreadEntryFunc(void * func);
        static void * InternalReceiverThreadEntryFunc(void * func);

        
        void startReceiver();
        //void startSenderID();

        void InternalReceiverEntry();
        void InternelSenderIDEntry();

        struct sockaddr_in  *m_CommunicatorModuleAddress = NULL; 
        int m_SocketFD = -1; 
        std::thread m_threadSenderID, m_threadCreateUDPSocket;
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