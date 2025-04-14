#ifndef     GLOBAL_H_
#define     GLOBAL_H_

#if !defined(UNUSED)
#define UNUSED(x) (void)(x) // Variables and parameters that are not used
#endif

#define BIT(x) (1 << (x))

#endif


#define MIN_RECONNECT_RATE_US   SEC_10 // 10 sec
#define DEFAULT_PING_RATE_US    SEC_2 // 2 sec
    

// SOCKET STATUS
#define SOCKET_STATUS_FRESH 			1   // socket is new
#define SOCKET_STATUS_CONNECTING    	2	// connecting to WS
#define SOCKET_STATUS_DISCONNECTING 	3   // disconnecting from WS
#define SOCKET_STATUS_DISCONNECTED 		4   // disconnected  from WS
#define SOCKET_STATUS_CONNECTED 		5   // connected to WS
#define SOCKET_STATUS_REGISTERED 		6   // connected and executed AddMe
#define SOCKET_STATUS_UNREGISTERED 		7   // connected but not registred
#define SOCKET_STATUS_ERROR 		    8   // Error


#if defined (DEBUG)

//  #define DDEBUG
//  #define DDEBUG_PARSER
//  #define DDEBUG_MSG
#endif