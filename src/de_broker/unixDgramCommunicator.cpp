#include "unixDgramCommunicator.hpp"
#include "common/chunk_protocol.hpp"
#include "../helpers/colors.hpp"
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <chrono>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <stdexcept>

namespace de {
namespace comm {

CUnixDgramCommunicator::~CUnixDgramCommunicator() {
    stop();
}

void CUnixDgramCommunicator::init(const char* socketPath, int chunkSize) {
    m_chunkSize = chunkSize;
    
#ifdef DEBUG_UNIX
    std::cout << _INFO_CONSOLE_TEXT << "UnixDgramCommunicator::init - socket:" << socketPath << " chunkSize:" << chunkSize << _NORMAL_CONSOLE_TEXT_ << std::endl;
#endif
    
    // Create communicator socket address
    memset(m_CommunicatorModuleAddress.get(), 0, sizeof(struct sockaddr_un));
    m_CommunicatorModuleAddress->sun_family = AF_UNIX;
    strncpy(m_CommunicatorModuleAddress->sun_path, socketPath, sizeof(m_CommunicatorModuleAddress->sun_path) - 1);
    
    // Remove stale socket file if exists
    unlink(socketPath);
}

void CUnixDgramCommunicator::start() {
#ifdef DEBUG_UNIX
    std::cout << _INFO_CONSOLE_TEXT << "UnixDgramCommunicator::start - creating socket" << _NORMAL_CONSOLE_TEXT_ << std::endl;
#endif
    // Create Unix domain socket
    m_SocketFD = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (m_SocketFD < 0) {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Failed to create Unix socket: " << strerror(errno) << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return;
    }
    
    // Bind to socket path
    if (bind(m_SocketFD, (struct sockaddr*)m_CommunicatorModuleAddress.get(), sizeof(struct sockaddr_un)) < 0) {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Failed to bind Unix socket: " << strerror(errno) << _NORMAL_CONSOLE_TEXT_ << std::endl;
        close(m_SocketFD);
        m_SocketFD = -1;
        return;
    }
    
#ifdef DEBUG_UNIX
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "UnixDgramCommunicator::start - socket bound to " << m_CommunicatorModuleAddress->sun_path << _NORMAL_CONSOLE_TEXT_ << std::endl;
#endif
    
    startReceiver();
    
    m_started = true;
}

void CUnixDgramCommunicator::stop() {
#ifdef DEBUG_UNIX
    std::cout << _INFO_CONSOLE_TEXT << "UnixDgramCommunicator::stop called" << _NORMAL_CONSOLE_TEXT_ << std::endl;
#endif
    if (!m_started) return;
    
    m_stopped_called = true;
    
    // Shutdown socket to unblock recvfrom() in receiver thread
    if (m_SocketFD >= 0) {
#ifdef DEBUG_UNIX
        std::cout << _INFO_CONSOLE_TEXT << "UnixDgramCommunicator::stop - shutting down socket" << _NORMAL_CONSOLE_TEXT_ << std::endl;
#endif
        shutdown(m_SocketFD, SHUT_RDWR);
    }
    
    if (m_threadCreateUnixSocket.joinable()) {
        m_threadCreateUnixSocket.join();
    }
    
    if (m_SocketFD >= 0) {
        close(m_SocketFD);
        m_SocketFD = -1;
    }
    
    // Remove socket file
    unlink(m_CommunicatorModuleAddress->sun_path);
    
    m_started = false;
#ifdef DEBUG_UNIX
    std::cout << _INFO_CONSOLE_TEXT << "UnixDgramCommunicator::stop completed" << _NORMAL_CONSOLE_TEXT_ << std::endl;
#endif
}

void CUnixDgramCommunicator::SendMsg(const char* message, const std::size_t datalength, struct sockaddr_un* module_address) {
#ifdef DEBUG_UNIX
    std::cout << _INFO_CONSOLE_TEXT << "UnixDgramCommunicator::SendMsg - length:" << datalength << " to:" << module_address->sun_path << _NORMAL_CONSOLE_TEXT_ << std::endl;
#endif
    
    std::lock_guard<std::mutex> lock(m_lock);

    try {
        int remainingLength = datalength;
        int offset = 0;
        uint16_t chunk_number = 0;

        while (remainingLength > 0) {
            int chunkLength = std::min(m_chunkSize, remainingLength);
            remainingLength -= chunkLength;
            
            // Create a new message with the chunk size + 2 * sizeof(uint8_t)
            char chunkMsg[chunkLength + 2 * sizeof(uint8_t)];

            // Prepare chunk header using shared helper
            chunk_protocol::prepareChunkHeader(chunk_number, (remainingLength == 0), 
                                              reinterpret_cast<uint8_t*>(chunkMsg));

#ifdef DEBUG_UNIX
            std::cout << "chunkNumber:" << chunk_number << " :chunkLength :" << chunkLength << std::endl;
#endif

            // Copy the chunk data into the message
            std::memcpy(chunkMsg + 2 * sizeof(uint8_t), message + offset, chunkLength);

            const int sent = sendto(m_SocketFD, chunkMsg, chunkLength + 2 * sizeof(uint8_t),
                MSG_CONFIRM, (const struct sockaddr*) module_address,
                sizeof(struct sockaddr_un));

            if (sent < 0) {
                std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "sendto failed: " << strerror(errno) << _NORMAL_CONSOLE_TEXT_ << std::endl;
                break;
            }

#ifdef DEBUG_UNIX
            std::cout << _INFO_CONSOLE_TEXT << "sent " << sent << " bytes" << _NORMAL_CONSOLE_TEXT_ << std::endl;
#endif

            if (remainingLength != 0)
            {
                // fast sending causes packet loss even on Unix domain sockets
                // (bounded SO_RCVBUF can overflow just like UDP).
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            offset += chunkLength;
            chunk_number++;
        }
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}

void CUnixDgramCommunicator::startReceiver() {
    m_threadCreateUnixSocket = std::thread(&CUnixDgramCommunicator::InternalReceiverEntry, this);
}

void CUnixDgramCommunicator::InternalReceiverEntry() {
#ifdef DDEBUG        
    std::cout << __PRETTY_FUNCTION__ << " line:" << __LINE__ << "  " << _LOG_CONSOLE_TEXT << "DEBUG: InternalReceiverEntry" << _NORMAL_CONSOLE_TEXT_ << std::endl;
#endif
    
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(m_SocketFD, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    struct sockaddr_un cliaddr;
    int n;
   
    std::map<std::string, std::vector<std::vector<uint8_t>>> receivedChunksBySource;

    while (!m_stopped_called) {
        __socklen_t sender_address_size = sizeof(cliaddr);
        n = recvfrom(m_SocketFD, (char*)m_buffer, MAXLINE,
                MSG_WAITALL, (struct sockaddr*) &cliaddr, &sender_address_size);
        
#ifdef DEBUG_UNIX
        std::cout << _INFO_CONSOLE_TEXT << "UnixDgramCommunicator::recvfrom - received " << n << " bytes from " << cliaddr.sun_path << _NORMAL_CONSOLE_TEXT_ << std::endl;
#endif
        
        if (n > 0) {
            if (n < 2) {
                std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Received packet too small: " << n << " bytes" << _NORMAL_CONSOLE_TEXT_ << std::endl;
                continue;
            }

            // Parse chunk number using shared helper
            const uint16_t chunkNumber = chunk_protocol::parseChunkNumber(reinterpret_cast<uint8_t*>(m_buffer));
#ifdef DEBUG_UNIX        
            std::cout << "chunkNumber:" << chunkNumber << " :len :" << n << std::endl;
#endif
            if (chunkNumber == 0) {
                // clear any corrupted/incomplete packets
                receivedChunksBySource[cliaddr.sun_path].clear();
            }
            
            // Last packet is always equal to 0xFFFF regardless of its actual number.
            const bool end = chunk_protocol::isEndChunk(chunkNumber);

#ifdef DEBUG_UNIX
            std::cout << _INFO_CONSOLE_TEXT << "chunkNumber:" << chunkNumber << " end:" << end << " size:" << n << _NORMAL_CONSOLE_TEXT_ << std::endl;
#endif

            // Store the received chunk in the map
            std::vector<std::vector<uint8_t>>& chunkVector = receivedChunksBySource[cliaddr.sun_path];
            chunkVector.emplace_back(m_buffer + 2 * sizeof(uint8_t), m_buffer + n);

            // Check if we have received all the chunks
            if (end) {
#ifdef DEBUG_UNIX
                std::cout << _INFO_CONSOLE_TEXT << "Reassembling " << chunkVector.size() << " chunks from " << cliaddr.sun_path << _NORMAL_CONSOLE_TEXT_ << std::endl;
#endif
                // Reassemble chunks using shared helper
                std::vector<uint8_t> concatenatedData = chunk_protocol::reassembleChunks(chunkVector);
                concatenatedData.push_back(0);

#ifdef DEBUG_UNIX
                std::cout << _INFO_CONSOLE_TEXT << "Calling callback with " << concatenatedData.size() << " bytes" << _NORMAL_CONSOLE_TEXT_ << std::endl;
#endif
                // Call the onReceive callback with the concatenated data
                if (m_callback != nullptr) {
                    m_callback->onReceive((const char*) concatenatedData.data(), concatenatedData.size(), &cliaddr);
                }

                // Clear the map for the next set of chunks
                receivedChunksBySource[cliaddr.sun_path].clear();
            }
        }
        else {
#ifdef DEBUG_UNIX
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "recvfrom failed: " << strerror(errno) << _NORMAL_CONSOLE_TEXT_ << std::endl;
            }
#endif
            if (m_stopped_called)
                break;
        }
    }

#ifdef DEBUG_UNIX
    std::cout << __PRETTY_FUNCTION__ << " line:" << __LINE__ << "  " << _LOG_CONSOLE_TEXT << "DEBUG: InternalReceiverEntry EXIT" << _NORMAL_CONSOLE_TEXT_ << std::endl;
#endif
}

} // namespace comm
} // namespace de
