
#include <iostream>
#include <stdint.h>
#include <unistd.h>
#include <vector>


#include "../../hal/gpio.hpp"
#include "../../notification_module/buzzer.hpp"

int main ()
{
    notification::CBuzzer &cBuzzer = notification::CBuzzer::getInstance();

    std::vector<notification::PORT_STATUS> buzzer_pins=
        {
            {16,GPIO_OFF},
            {20,GPIO_OFF},
            {21,GPIO_OFF},
        };

    cBuzzer.init(buzzer_pins);
    cBuzzer.switchBuzzer(0,true,notification::CBuzzer::BARO_BUZZ,10);
    
    //cBuzzer.update();
    bool O = true;
    while (true)
    {
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