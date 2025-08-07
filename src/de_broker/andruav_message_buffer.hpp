#ifndef ANDRUAV_MESSAGE_BUFFER_H_
#define ANDRUAV_MESSAGE_BUFFER_H_

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <memory>
#include <iostream>
#include <utility> 
#include <netinet/in.h> 


struct MessageWithSocket {
    std::string message;
    struct sockaddr_in socket;
};

namespace de
{
namespace comm
{

    class CMessageBuffer {
        public:
            void enqueue(const MessageWithSocket& msgWithSocket);

            MessageWithSocket dequeue();

        private:
            std::queue<MessageWithSocket> m_queue;
            std::mutex m_mutex;
            std::condition_variable m_cond;
            uint32_t m_message_count =0;
    };
}
}

#endif
