#ifndef NOTIFICATION_H
#define NOTIFICATION_H


#include <vector>

#include "../status.hpp"

namespace notification
{

#define LED_STATUS_ON   1
#define LED_STATUS_OFF  0
#define LED_STATUS_FLASHING   2


typedef struct  
{
    std::string name;
    uint8_t gpio_pin;
    uint8_t status;
} PORT_STATUS;

enum class ENUM_Module_Error_Code
{
    ERR_UNINITIALIZED   = -1,
    ERR_NON             = 0,
    ERR_NO_HW_AVAILABLE = 1,
    ERR_INIT_FAILED     = 2,
    ERR_UNKNOWN         = 999
};

class CNotification
{

    public:

        virtual bool init (const std::vector<PORT_STATUS>& led_pins) = 0;
        virtual void update()   = 0;
        virtual void uninit()   {};
        
        virtual const std::vector<PORT_STATUS> getPorts() const 
        {
            return m_port_pins;
        }

        virtual ENUM_Module_Error_Code getStatus()
        {
            return m_error;
        }

    protected:
        ENUM_Module_Error_Code m_error = ENUM_Module_Error_Code::ERR_UNINITIALIZED;

        std::vector<PORT_STATUS> m_port_pins;

        uavos::STATUS &m_status = uavos::STATUS::getInstance();

};

};
#endif
