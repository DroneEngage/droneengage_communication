#include <memory>
#include <utility>

#include "../helpers/colors.hpp"
#include "../helpers/helpers.hpp"

#include "andruav_message_buffer.hpp"

using namespace de::comm;

// Pass by r-value reference and move to the queue to avoid copy construction
void CMessageBuffer::enqueue(std::unique_ptr<MessageWithSocket> msgWithSocket) {
    
    // The lock is only needed for the queue modification and condition variable notification.
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push(std::move(msgWithSocket));
    m_cond.notify_one();
}

std::unique_ptr<MessageWithSocket> CMessageBuffer::dequeue() {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    // Wait for the queue to be non-empty.
    m_cond.wait(lock, [this]() { 
        return !m_queue.empty(); });
    
    std::unique_ptr<MessageWithSocket> message_ptr = std::move(m_queue.front());
    m_queue.pop();
    
    m_message_count.fetch_sub(1, std::memory_order_relaxed);
    
    return message_ptr;
}

std::unique_ptr<MessageWithSocket> CMessageBuffer::peek_and_pop() {
    std::unique_lock<std::mutex> lock(m_mutex, std::defer_lock);
    
    // Non-blocking lock attempt. Returns immediately if the lock can't be acquired.
    if (!lock.try_lock()) {
        return nullptr;
    }
    
    if (m_queue.empty()) {
        return nullptr;
    }
    
    std::unique_ptr<MessageWithSocket> message_ptr = std::move(m_queue.front());
    m_queue.pop();
    
    m_message_count.fetch_sub(1, std::memory_order_relaxed);
    
    return message_ptr;
}