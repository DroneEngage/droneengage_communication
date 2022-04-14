#ifndef HAL_STATUS_H
#define HAL_STATUS_H

namespace uavos 
{
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

            inline bool is_fcb_module_connected() const
            {
                return m_fcb_module_connected;
            }

            inline void is_fcb_module_connected(const bool fcb_module_connected)
            {
                m_fcb_module_connected = fcb_module_connected;
            }

            inline bool is_camera_module_connected() const
            {
                return m_camera_module_connected;
            }

            inline void is_camera_module_connected(const bool camera_module_connected)
            {
                m_camera_module_connected = camera_module_connected;
            }

            inline bool is_light_connected() const
            {
                return m_light;
            }

            inline void is_light_connected(const bool light)
            {
                m_light = light;
            }

            inline bool is_buzzer_connected() const
            {
                return m_buzzer;
            }

            inline void is_buzzer_connected(const bool buzzer)
            {
                m_buzzer = buzzer;
            }

            // std::bool is_online(const std::bool online)
            // {
            //     ;
            // }
            bool m_exit_me = false;
        private:

            bool m_online = false;
            bool m_fcb_module_connected = false;
            bool m_camera_module_connected = false;
            bool m_buzzer = false;
            bool m_light = false;

           
            
};

};

#endif
