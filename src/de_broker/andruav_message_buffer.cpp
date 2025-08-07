
#include "../helpers/colors.hpp"
#include "../helpers/helpers.hpp"

#include "andruav_message_buffer.hpp"

using namespace de::comm;



void CMessageBuffer::enqueue(const MessageWithSocket& msgWithSocket) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push(msgWithSocket);
    #ifdef DEBUG
        m_message_count++;
        if (m_message_count%100)
        {
            std::cout  << _LOG_CONSOLE_BOLD_TEXT << "Msg-Q:" << _INFO_CONSOLE_BOLD_TEXT << m_message_count << _NORMAL_CONSOLE_TEXT_ << std::endl;
        }
    #endif
    m_cond.notify_one(); // Notify the consumer thread
}


MessageWithSocket CMessageBuffer::dequeue() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait(lock, [this]() { 
        #ifdef DEBUG
            std::cout  << _LOG_CONSOLE_BOLD_TEXT << "No more messages in Q" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif
        return !m_queue.empty(); }
    ); // Wait until there is a message
    MessageWithSocket message = m_queue.front();
    m_queue.pop();
    return message;
}
