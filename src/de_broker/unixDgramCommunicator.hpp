#ifndef CUNIXDGRAMCOMMUNICATOR_H
#define CUNIXDGRAMCOMMUNICATOR_H

#include <thread>
#include <mutex>
#include <sys/un.h>

#define MAX_UNIX_DGRAM_PACKET_SIZE 0xffff
#define DEFAULT_UNIX_DGRAM_PACKET_SIZE 8192

typedef void (*ONRECEIVE_CALLBACK_UNIX)(const char *, int len, struct sockaddr_un * sock);

#ifndef MAXLINE
#define MAXLINE 0xffff
#endif

namespace de {
namespace comm {

class CCallBack_UnixDgramCommunicator {
    public:
        virtual void onReceive(const char *, int len, struct sockaddr_un * sock) {};
};

class CUnixDgramCommunicator {
    public:
        CUnixDgramCommunicator(CCallBack_UnixDgramCommunicator* callback) {
            m_callback = callback;
        }

        ~CUnixDgramCommunicator();

    public:
        void init(const char* socketPath, int chunkSize);
        void start();
        void stop();
        void SendMsg(const char* message, const std::size_t datalength, struct sockaddr_un* module_address);

    protected:
        void startReceiver();
        void InternalReceiverEntry();

        int m_SocketFD = -1;
        std::thread m_threadCreateUnixSocket;

    protected:
        bool m_started = false;
        bool m_stopped_called = false;
        std::mutex m_lock;

        CCallBack_UnixDgramCommunicator* m_callback = nullptr;
        std::unique_ptr<struct sockaddr_un> m_CommunicatorModuleAddress = std::make_unique<struct sockaddr_un>();
        int m_chunkSize;

        char m_buffer[MAXLINE];
};

} // namespace comm
} // namespace de

#endif // CUNIXDGRAMCOMMUNICATOR_H
