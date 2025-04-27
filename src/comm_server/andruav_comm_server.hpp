#ifndef ANDRUAV_COMM_SERVER_H_
#define ANDRUAV_COMM_SERVER_H_

#include "andruav_unit.hpp"
#include "andruav_comm_ws.hpp"
#include "andruav_comm_server_base.hpp" // Include the base class header
#include "../global.hpp"

namespace de
{
namespace andruav_servers
{
    class CAndruavCommServer : public std::enable_shared_from_this<CAndruavCommServerBase>, public CCallBack_WSASession, public CAndruavCommServerBase
    {
    public:
        static CAndruavCommServer& getInstance()
        {
            static CAndruavCommServer instance;
            return instance;
        };

    private:
        CAndruavCommServer() : CAndruavCommServerBase() // Call the base class constructor
        {
        }

        CAndruavCommServer(CAndruavCommServer const&) = delete;
        void operator=(CAndruavCommServer const&) = delete;

    public:
        ~CAndruavCommServer() override = default;

    public:
        void start() override;
        void connect() override;
        void uninit(const bool exit) override;
        
        void onSocketError() override;
        void onBinaryMessageRecieved(const char* message, const std::size_t datalength) override;
        void onTextMessageRecieved(const std::string& jsonMessage) override;

    public:
        void API_pingServer() override;
        void API_sendSystemMessage(const int command_type, const Json_de& msg) const override;
        void API_sendCMD(const std::string& target_name, const int command_type, const Json_de& msg) override;
        std::string API_sendCMDDummy(const std::string& target_name, const int command_type, const Json_de& msg) override;
        void API_sendBinaryCMD(const std::string& target_party_id, const int command_type, const char* bmsg, const uint64_t bmsg_length, const Json_de& message_cmd) override;
        void sendMessageToCommunicationServer(const char* full_message, const std::size_t full_message_length, const bool& is_system, const bool& is_binary, const std::string& target_id, const int msg_type, const Json_de& msg_cmd) override;



    private:
        void startWatchDogThread() override;
        void connectToCommServer(const std::string& server_ip, const std::string& server_port, const std::string& key) override;

    private:
        de::andruav_servers::CWSAProxy& _cwsa_proxy = de::andruav_servers::CWSAProxy::getInstance();
        std::unique_ptr<de::andruav_servers::CWSASession> _cwsa_session;

        CAndruavUnits& m_andruav_units = CAndruavUnits::getInstance();
        de::comm::CAndruavMessage& m_andruav_message = de::comm::CAndruavMessage::getInstance();
    };
}
}

#endif