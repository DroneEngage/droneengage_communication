#ifndef HAL_STATUS_H
#define HAL_STATUS_H


#include "./comm_server/andruav_unit.hpp"

namespace de 
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

            inline const bool is_p2p_connected() const
            {
                de::CAndruavUnitMe& andruavMe = de::CAndruavUnitMe::getInstance();
                de::DE_UNIT_P2P_INFO& andruav_unit_p2p_me = andruavMe.getUnitP2PInfo();
                return andruav_unit_p2p_me.is_p2p_connected;
            }

            inline void is_p2p_connected(const bool is_p2p_connected) 
            {
                de::CAndruavUnitMe& andruavMe = de::CAndruavUnitMe::getInstance();
                de::DE_UNIT_P2P_INFO& andruav_unit_p2p_me = andruavMe.getUnitP2PInfo();
                andruav_unit_p2p_me.is_p2p_connected = is_p2p_connected;
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


            inline void cpu_temp (const uint32_t cpu_temprature)
            {
                m_cpu_temprature = cpu_temprature;
            }

            uint32_t cpu_temp () const 
            {
                return m_cpu_temprature;
            }

            
            inline void cpu_status (const uint32_t cpu_status)
            {
                m_cpu_status = cpu_status;
            }

            uint32_t cpu_status () const 
            {
                return m_cpu_status;
            }

            bool cpu_undervoltage_detected ()
            {
                return (m_cpu_status & 0x01) != 0;
            }

            bool cpu_undervoltage_occured ()
            {
                return (m_cpu_status & 0x10000) != 0;
            }

            inline void streaming_level (int streaming_level) 
            {
                m_streaming_level = streaming_level;
            }
            
            inline int streaming_level () const 
            {
                return m_streaming_level;
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

            uint32_t m_cpu_temprature=-1; // unavailable

            /**
             * @brief 
             * Bit	    Hex value	Meaning
             * 0        0x1         Under-voltage detected
             * 1        0x2         Arm frequency capped
             * 2        0x4         Currently throttled
             * 3        0x8         Soft temperature limit active
             * 16       0x10000     Under-voltage has occurred
             * 17       0x20000     Arm frequency capping has occurred
             * 18       0x40000     Throttling has occurred
             * 19       0x80000     Soft temperature limit has occurred
             */
            uint32_t m_cpu_status;
            
            int m_streaming_level =-1;
           
            
};

};

#endif
