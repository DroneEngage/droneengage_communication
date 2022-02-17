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

            inline bool is_online() const
            {
                return uavos::andruav_servers::CAndruavCommServer::getInstance().getStatus()==SOCKET_STATUS_REGISTERED;
            }


            inline bool is_fcb_connected() const
            {
                return m_fcb_connected;
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
