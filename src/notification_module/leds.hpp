#ifndef HAL_LED_MODULE_H
#define HAL_LED_MODULE_H

/**
 * @file leds.hpp
 * @author Mohammad Hefny
 * @brief LED ON/OFF & Toggle Driver
 * @version 0.1
 * @date 2022-02-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "notification.hpp"
#include "../status.hpp"

#define LED_ON  1
#define LED_OFF 0
namespace notification
{


class CLEDs : public CNotification
{
    public:
        //https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
        static CLEDs& getInstance()
        {
            static CLEDs instance;

            return instance;
        }

        CLEDs(CLEDs const&)                  = delete;
        void operator=(CLEDs const&)         = delete;

    private:

        CLEDs()
        {
            m_counter = 0;
        }

    public:
        
        ~CLEDs (){};

    public:

        bool init () override;
        void update() override;
        void uninit() override;  
        
        
        void switchLED(const uint8_t led_index, const bool onOff);

    private:
        uint32_t m_counter;
        de::STATUS &m_status = de::STATUS::getInstance();

    private:
        int m_power_led_on = 0;
                 
};

}

#endif

