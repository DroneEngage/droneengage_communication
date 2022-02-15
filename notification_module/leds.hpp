#ifndef HAL_LED_MODULE_H
#define HAL_LED_MODULE_H

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
        void update() override;

        
};

}

#endif

