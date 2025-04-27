#ifndef ANDRUAV_COMM_SERVER_BASE_H_
#define ANDRUAV_COMM_SERVER_BASE_H_

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <pthread.h>

#include "../global.hpp"
#include "../de_broker/andruav_message.hpp"
#include "../helpers/json_nlohmann.hpp"

using Json_de = nlohmann::json;

namespace de
{
namespace andruav_servers
{
    class CAndruavCommServerBase : public std::enable_shared_from_this<CAndruavCommServerBase>
    {
    public:
        virtual ~CAndruavCommServerBase() = default;

    protected:
        CAndruavCommServerBase() : m_exit(false), m_next_connect_time(0), m_lasttime_access(0), m_on_off_delay(0), m_status(SOCKET_STATUS_FRESH) {}

    
    public:
        virtual void turnOnOff(const bool on_off, const uint32_t duration_seconds);
        
    public:
        virtual void start () {};
        virtual void connect () {};
        virtual void start(const std::string comm_server_ip, const uint16_t comm_server_port, const std::string comm_server_key) {};
        virtual void uninit(const bool exit) = 0;
        
        virtual void onSocketError() = 0;
        virtual void onBinaryMessageRecieved(const char* message, const std::size_t datalength) = 0;
        virtual void onTextMessageRecieved(const std::string& jsonMessage) = 0;

        virtual void API_pingServer() = 0;
        virtual void API_sendSystemMessage(const int command_type, const Json_de& msg) const = 0;
        virtual void API_sendCMD(const std::string& target_name, const int command_type, const Json_de& msg) = 0;
        virtual std::string API_sendCMDDummy(const std::string& target_name, const int command_type, const Json_de& msg) = 0;
        virtual void API_sendBinaryCMD(const std::string& target_party_id, const int command_type, const char* bmsg, const uint64_t bmsg_length, const Json_de& message_cmd) = 0;
        virtual void sendMessageToCommunicationServer(const char* full_message, const std::size_t full_message_length, const bool& is_system, const bool& is_binary, const std::string& target_id, const int msg_type, const Json_de& msg_cmd) = 0;
        virtual int getStatus() const
        {
            return m_status;
        }

        const bool& shouldExit() const
        {
            return m_exit;
        }

        inline const u_int64_t getLastTimeAccess() const
        {
            return m_lasttime_access;
        }

    protected:
        virtual void switchOnline();
        virtual void switchOffline();

        virtual void startWatchDogThread() = 0;
        virtual void connectToCommServer(const std::string& server_ip, const std::string& server_port, const std::string& key) = 0;

        
    protected:
        std::string m_url_param;
        std::string m_host;
        std::string m_port;
        std::string m_party_id;

        u_int8_t m_status;

        u_int64_t m_next_connect_time;
        u_int64_t m_lasttime_access;

        u_int64_t m_on_off_delay;

        std::unique_ptr<std::thread> m_watch_dog;
        bool m_exit;
    };
};
};

#endif