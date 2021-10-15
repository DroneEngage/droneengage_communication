#include <iostream>
#include <cstring> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <unistd.h>

#include "./helpers/colors.hpp"
#include "./helpers/json.hpp"
using Json = nlohmann::json;

#include "udpCommunicator.hpp"


#define MAXLINE 8192 
char buffer[MAXLINE]; 
    
uavos::comm::CUDPCommunicator::~CUDPCommunicator ()
{
    
    #ifdef DEBUG
	std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: ~CUDPCommunicator" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    if (m_stopped_called == false)
    {
        #ifdef DEBUG
	    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: ~CUDPCommunicator" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif

        stop();
    }

    #ifdef DEBUG
	std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: ~CUDPCommunicator" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    // destroy mutex
	//pthread_mutex_destroy(&m_lock);

    #ifdef DEBUG
	std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: ~CUDPCommunicator" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

}

/**
 * @brief 
 * 
 * @param host address of Communicator
 * @param listenningPort port of communicator
 */
void uavos::comm::CUDPCommunicator::init (const char * host, int listenningPort)
{

    // pthread initialization
	m_thread = pthread_self(); // get pthread ID
	pthread_setschedprio(m_thread, SCHED_FIFO); // setting priority


    // Creating socket file descriptor 
    if ( (m_SocketFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    }

    m_CommunicatorModuleAddress = new (struct sockaddr_in)();
    memset(m_CommunicatorModuleAddress, 0, sizeof(struct sockaddr_in)); 
     
    // Communication Server (IP - PORT) 
    m_CommunicatorModuleAddress->sin_family = AF_INET; 
    m_CommunicatorModuleAddress->sin_port = htons(listenningPort); 
    m_CommunicatorModuleAddress->sin_addr.s_addr = inet_addr(host);//INADDR_ANY; 
    
    
    // Bind the socket with the server address 
    if (bind(m_SocketFD, (const struct sockaddr *)m_CommunicatorModuleAddress, sizeof(struct sockaddr_in)) < 0 ) 
    { 
        std::cout << "UDP Listener  " << _ERROR_CONSOLE_TEXT_ << " BAD BIND: " << host << ":" << listenningPort << _NORMAL_CONSOLE_TEXT_ << std::endl;
        exit(-1) ;
    } 
}

void uavos::comm::CUDPCommunicator::start()
{
    #ifdef DEBUG        
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: start" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    // call directly as we are already in a thread.
    if (m_starrted == true)
        throw "Starrted called twice";

    startReceiver ();
    //startSenderID();

    m_starrted = true;
}


void uavos::comm::CUDPCommunicator::startReceiver ()
{
    m_threadCreateUDPSocket = std::thread (this->InternalReceiverThreadEntryFunc, (void *) this);
}


// void uavos::comm::CUDPCommunicator::startSenderID ()
// {
//     //m_threadSenderID = std::thread (this->InternalSenderIDThreadEntryFunc, (void *) this);
// }


void uavos::comm::CUDPCommunicator::stop()
{
    #ifdef DEBUG
	std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: Stop" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    m_stopped_called = true;

    if (m_SocketFD != -1)
        shutdown(m_SocketFD, SHUT_RDWR);
    
    #ifdef DEBUG
	std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: Stop Socket Closed" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    try
    {
        //pthread_join(m_threadSenderID, NULL); 	// close the thread
        //pthread_join(m_threadCreateUDPSocket, NULL); 	// close the thread
        m_threadCreateUDPSocket.join();
        //m_threadSenderID.join();
        //pthread_join(m_thread, NULL); 	// close the thread
        //close(m_SocketFD); 					// close UDP socket
        delete m_CommunicatorModuleAddress;
        //delete m_CommunicatorModuleAddress;

        #ifdef DEBUG
	    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: Stop Threads Killed" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif
    }
    catch(...)
    {
        //std::cerr << e.what() << '\n';
    }

    #ifdef DEBUG
	std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: Stop out" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}


void uavos::comm::CUDPCommunicator::InternalReceiverEntry()
{
    #ifdef DEBUG        
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: InternalReceiverEntry" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    // https://stackoverflow.com/questions/2876024/linux-is-there-a-read-or-recv-from-socket-with-timeout
    struct timeval tv;
    tv.tv_sec = 1; // timeout_in_seconds;
    tv.tv_usec = 0;
    setsockopt(m_SocketFD, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    struct sockaddr_in  cliaddr;
    int n;
    __socklen_t sender_address_size = sizeof (cliaddr);
   
    while (!m_stopped_called)
    {
        // TODO: you should send header ot message length and handle if total message size is larger than MAXLINE.
        n = recvfrom(m_SocketFD, (char *)buffer, MAXLINE,  
                MSG_WAITALL, ( struct sockaddr *) &cliaddr, &sender_address_size);
        
        if (n > 0) 
        {

            buffer[n]=0; // make it zero-terminated
            if (m_OnReceive != NULL)
            {
                m_OnReceive((const char *) buffer, n, &cliaddr);
            } 
        }

        
    }

    #ifdef DEBUG
	std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: InternalReceiverEntry EXIT" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}


void uavos::comm::CUDPCommunicator::SetMessageOnReceive ( ONRECEIVE_CALLBACK onReceive)
{
    m_OnReceive = onReceive;
}


void * uavos::comm::CUDPCommunicator::InternalReceiverThreadEntryFunc(void * This) {
	((CUDPCommunicator *)This)->InternalReceiverEntry(); 
    return NULL;
}


/**
 * Sends JMSG to Communicator
 **/
void uavos::comm::CUDPCommunicator::SendMsg(const char * message, const std::size_t datalength, struct sockaddr_in * module_address)
{
    #ifdef DEBUG        
     //   std::cout << _LOG_CONSOLE_TEXT << "SendMsg: " << jmsg << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    try
    {
    sendto(m_SocketFD, message, datalength,  
        MSG_CONFIRM, (const struct sockaddr *) module_address, 
            sizeof(struct sockaddr_in));         
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}