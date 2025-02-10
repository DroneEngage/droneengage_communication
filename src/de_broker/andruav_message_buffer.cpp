

#include "andruav_message_buffer.hpp"

using namespace de::comm;



void CMessageBuffer::enqueue(const MessageWithSocket& msgWithSocket) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push(msgWithSocket);
    m_cond.notify_one(); // Notify the consumer thread
}


MessageWithSocket CMessageBuffer::dequeue() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait(lock, [this]() { return !m_queue.empty(); }); // Wait until there is a message
    MessageWithSocket message = m_queue.front();
    m_queue.pop();
    return message;
}
