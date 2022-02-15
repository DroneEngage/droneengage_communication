#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <stdint.h>

namespace hal
{

#define HAL_GPIO_INPUT  0
#define HAL_GPIO_OUTPUT 1
#define HAL_GPIO_ALT    2

class CGPIO
{

    public:
        


        CGPIO (CGPIO const &)           = delete;
        void operator=(CGPIO const &)   = delete;
    
    protected:
        CGPIO() {};

        
    public:
        ~CGPIO() {};
    
    public:
        
        virtual void pinMode (uint8_t pin, uint8_t output) = 0;
        virtual uint8_t read (uint8_t pin) = 0;

};

};


#endif