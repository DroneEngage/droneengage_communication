
#include <stdint.h>
#include <unistd.h>
#include <vector>


#include "../hal/gpio.hpp"
#include "../notification_module/buzzer.hpp"

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
    cBuzzer.update();
    cBuzzer.update();
    bool O = true;
    while (true)
    {
        cBuzzer.on(0,O);
        cBuzzer.on(1,!O);
        O != O;
        usleep(100000); // 10Hz
    }
}