#ifndef HAL_STATUS_H
#define HAL_STATUS_H

#include "comm_server/andruav_comm_server.hpp"

class STATUS {
    
    public:

        static STATUS& getInstance()
            {
                static STATUS instance;

                return instance;
            }

            STATUS(STATUS const&)                  = delete;
            void operator=(STATUS const&)          = delete;

        private:

            STATUS()
            {

            }

        public:
            
            ~STATUS (){};

        public:

            bool is_online()
            {
                return uavos::andruav_servers::CAndruavCommServer::getInstance().getStatus()==SOCKET_STATUS_REGISTERED;
            }

            // std::bool is_online(const std::bool online)
            // {
            //     ;
            // }
            bool m_fcb_connected = false;
            bool m_exit_me = false;
        private:



            //bool m_is_online = false;
        
};


#endif
