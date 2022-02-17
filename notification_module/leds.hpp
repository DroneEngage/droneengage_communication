#ifndef HAL_LED_MODULE_H
#define HAL_LED_MODULE_H

#include <vector>
#include "notification.hpp"

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

        }

    public:
        
        ~CLEDs (){};

    public:

        void init() override;  
        void init (const std::vector<uint8_t>& led_pins);
        void update() override;
        void uninit() override;  
        
        
        int getLEDS() const;
        void switchLED(const uint8_t led_index, const bool onOff);

    private:

        std::vector<uint8_t> m_led_pins;
};

}

#endif

