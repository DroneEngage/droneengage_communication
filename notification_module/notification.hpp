#ifndef NOTIFICATION_H
#define NOTIFICATION_H

namespace notification
{


enum class ENUM_Module_Error_Code
{
    ERR_UNINITIALIZED   = -1,
    ERR_NON             = 0,
    ERR_NO_HW_AVAILABLE = 1,
    ERR_UNKNOWN         = 999
};

class CNotification
{

    public:

        virtual void init()     = 0;  
        virtual void update()   = 0;

        virtual ENUM_Module_Error_Code getStatus()
        {
            return m_error;
        }

    protected:
        ENUM_Module_Error_Code m_error = ENUM_Module_Error_Code::ERR_UNINITIALIZED;
};

};
#endif
