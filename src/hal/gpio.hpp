#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <stdint.h>

namespace hal
{

#define HAL_GPIO_INPUT  0
#define HAL_GPIO_OUTPUT 1
#define HAL_GPIO_ALT    2


// Determine electrical sognal for LED to be ON or OFF i.e. 0 or 3.3v for output port.
#define GPIO_ON     1
#define GPIO_OFF    0

class CGPIO
{

    public:
        
        CGPIO (CGPIO const &)           = delete;
        void operator=(CGPIO const &)   = delete;
    
    protected:
        CGPIO() {};

        
    public:
        virtual ~CGPIO() {};
    
    public:
        
        virtual void pinMode (uint8_t pin, uint8_t output) = 0;
        virtual uint8_t read (uint8_t pin) = 0;
        virtual void write(uint8_t pin, uint8_t value) = 0;
        virtual uint8_t toggle(uint8_t pin) = 0;
        virtual bool init () = 0;
};

};


#endif