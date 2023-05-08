#ifndef ANDRUAV_COMM_WS_H_
#define ANDRUAV_COMM_WS_H_

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>         // std::thread

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


namespace uavos
{
namespace andruav_servers
{


class CCallBack_WSASession
{
    public:

    virtual void onBinaryMessageRecieved(const char * message, const std::size_t datalength)   {};                                                          
    virtual void onTextMessageRecieved(const std::string& JsonMessage)   {};                                                          
    virtual void onSocketClosed()                                   {}; 
    virtual void onSocketError ()                                   {};
    
};


class CWSASession 
{
        public:
            //https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
            
            
            CWSASession(boost::asio::io_context& io_context, const std::string& host, const std::string& port, const std::string& url_param, CCallBack_WSASession &callback): io_context_(io_context), host_(host), port_(port), url_param_(url_param), resolver_(io_context), ws_(io_context, ssl_ctx_), m_callback (callback)
            {
                
            };
        
        public:
            
            ~CWSASession (){
                if (m_thread_receiver.joinable()) {
                    m_thread_receiver.join();
                }
            };
            
        public:

            void run();

            void writeText (const std::string& message);
            void writeBinary (const char * bmsg, const int& length);
            void receive_message();
            void close();
            void close(beast::websocket::close_code code);
            void shutdown ();

        private:
            bool m_connected = false;
            boost::asio::io_context& io_context_;
            const std::string host_;
            const std::string port_;
            const std::string url_param_;
            tcp::resolver resolver_;
            ssl::context ssl_ctx_{ssl::context::tlsv12_client};
            websocket::stream<beast::ssl_stream<tcp::socket>> ws_;
            std::thread m_thread_receiver;
            uavos::andruav_servers::CCallBack_WSASession &m_callback;
};

class CWSAProxy
{
    public:
        
        //https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
        static CWSAProxy& getInstance()
        {
            static CWSAProxy instance;

            return instance;
        };

    public:
        CWSAProxy(CWSAProxy const&) = delete;
        void operator=(CWSAProxy const&) = delete;

    private:

        CWSAProxy() 
        {
        };
    
    public:
            
        ~CWSAProxy (){};

    public:
        std::unique_ptr<uavos::andruav_servers::CWSASession> run(char const* host, char const* port, char const* url_param, CCallBack_WSASession &callback);
        boost::asio::io_context io_context;
    
};
        
};
};


#endif
