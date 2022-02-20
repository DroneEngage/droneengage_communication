
#include <iostream>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>
#include <vector>


#include "../../hal/gpio.hpp"
#include "../../notification_module/buzzer.hpp"


void quit_handler( int sig )
{
    notification::CBuzzer::getInstance().uninit();	
}

int main ()
{

    signal(SIGINT,quit_handler);
    signal(SIGTERM,quit_handler);
	
    notification::CBuzzer &cBuzzer = notification::CBuzzer::getInstance();

    std::vector<notification::PORT_STATUS> buzzer_pins=
        {
            {16,GPIO_OFF},
            {20,GPIO_OFF},
            {21,GPIO_OFF},
        };

    cBuzzer.init(buzzer_pins);
    cBuzzer.switchBuzzer(1,true,notification::CBuzzer::EKF_BAD,10);
    
    //cBuzzer.update();
    bool O = true;
    int i = 500;
    while (i>0)
    {
        --i;
        cBuzzer.update();

        // std::cout << "0:" << std::to_string(O) << std::endl;
        // cBuzzer.on(1,O);
        // std::cout << "1:" << std::to_string(O) << std::endl;
        // cBuzzer.on(2,O);
        // std::cout << "2:" << std::to_string(O) << std::endl;
        // O = !O;
        usleep(100000); // 10Hz
    }
}