#ifndef HAL_STATUS_H
#define HAL_STATUS_H


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
                return m_online;
            }

            inline void is_online(const bool online) 
            {
                m_online = online; 
            }

            inline bool is_fcb_connected() const
            {
                return m_fcb_connected;
            }

            // std::bool is_online(const std::bool online)
            // {
            //     ;
            // }
            bool m_online = false;
            bool m_fcb_connected = false;
            bool m_exit_me = false;
        private:



            //bool m_is_online = false;
        
};


#endif
