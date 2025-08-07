#ifndef ANDRUAV_MESSAGE_BUFFER_H_
#define ANDRUAV_MESSAGE_BUFFER_H_

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <memory>
#include <iostream>
#include <utility>
#include <netinet/in.h>

struct MessageWithSocket {
    std::string message;
    struct sockaddr_in socket;
};

namespace de {
namespace comm {

    class CMessageBuffer {
    public:
        // Use std::unique_ptr for better memory management and exception safety.
        void enqueue(std::unique_ptr<MessageWithSocket> msgWithSocket);
        
        // Return a std::unique_ptr to avoid unnecessary copying.
        std::unique_ptr<MessageWithSocket> dequeue();
        
        // Peek and Pop: A function has been added for scenarios 
        // where a consumer thread might want to check for messages without blocking. 
        // It uses a try_lock to avoid blocking other threads and returns nullptr if the queue is empty. This is beneficial in non-critical paths or for event loops where blocking isn't desired.
        std::unique_ptr<MessageWithSocket> peek_and_pop();

    private:
        std::queue<std::unique_ptr<MessageWithSocket>> m_queue;
        std::mutex m_mutex;
        std::condition_variable m_cond;
        std::atomic<uint32_t> m_message_count {0}; // Atomic for lock-free access
    };
}
}
#endif