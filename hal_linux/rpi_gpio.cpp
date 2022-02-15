#include <iostream>
#include <cstdlib>
#include <unistd.h> //close
#include <fcntl.h> // O_RDWR

#include <sys/mman.h>
#include <sys/stat.h>

#include "../helpers/colors.hpp"
#include "../helpers/util_rpi.hpp"
#include "rpi_gpio.hpp"

using namespace hal_linux;

#define GPIO_RPI_MAX_NUMBER_PINS 32

const uint8_t CRPI_GPIO::_gpio_registers_memory_range = 0xB4;

CRPI_GPIO::CRPI_GPIO()
{
    
}




bool CRPI_GPIO::openMemoryDevice()
{
    std::cout << "openMemoryDevice" << std::endl;
        
    _system_memory_device = open(_system_memory_device_path.c_str(), O_RDWR|O_SYNC|O_CLOEXEC);
    if (_system_memory_device < 0) {
        return false;
    }

    return true;
}

void CRPI_GPIO::closeMemoryDevice()
{
    close(_system_memory_device);
    // Invalidate device variable
    _system_memory_device = -1;
}


void CRPI_GPIO::init()
{

    const int rpi_version = helpers::CUtil_Rpi::getInstance().get_rpi_model();
    if (rpi_version == -1) 
    {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Error: Cannot initialize GPIO because it is not RPI-Board" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return ;
    }
    
    std::cout << "B1" << std::endl;

    CRPI_GPIO::Address peripheral_base;
    
    if(rpi_version == 1) {
        peripheral_base = Address::BCM2708_PERIPHERAL_BASE;
    } else if (rpi_version == 2) {
        peripheral_base = Address::BCM2709_PERIPHERAL_BASE;
    } else {
        peripheral_base = Address::BCM2711_PERIPHERAL_BASE;
    }

    if (!openMemoryDevice()) {
        exit(1); //ASSERT("Failed to initialize memory device.");
        //return;
    }

    const uint32_t gpio_address = getAddress(peripheral_base, PeripheralOffset::GPIO);

    _gpio = get_memory_pointer(gpio_address, CRPI_GPIO::_gpio_registers_memory_range);
    if (!_gpio) {
        exit(1); //ASSERT("Failed to get GPIO memory map.");
    }

    // No need to keep mem_fd open after mmap
    closeMemoryDevice();

}

volatile uint32_t* CRPI_GPIO::get_memory_pointer(uint32_t address, uint32_t range) const
{
    auto pointer = mmap(
        nullptr,                         // Any adddress in our space will do
        range,                           // Map length
        PROT_READ|PROT_WRITE|PROT_EXEC,  // Enable reading & writing to mapped memory
        MAP_SHARED|MAP_LOCKED,           // Shared with other processes
        _system_memory_device,           // File to map
        address                          // Offset to GPIO peripheral
    );

    if (pointer == MAP_FAILED) {
        return nullptr;
    }

    std::cout << "get_memory_pointer 2" << std::endl;
    return static_cast<volatile uint32_t*>(pointer);
}


void CRPI_GPIO::pinMode (uint8_t pin, uint8_t output)
{
    if (output == HAL_GPIO_INPUT) {
        setGPIOModeIn(pin);
    } else {
        setGPIOModeIn(pin);
        setGPIOModeOut(pin);
    }
}


void CRPI_GPIO::setGPIOModeIn(int pin)
{
    // Each register can contain 10 pins
    const uint8_t pins_per_register = 10;
    // Calculates the position of the 3 bit mask in the 32 bits register
    const uint8_t tree_bits_position_in_register = (pin%pins_per_register)*3;
    // Create a mask that only removes the bits in this specific GPIO pin, E.g:
    // 0b11'111'111'111'111'111'111'000'111'111'111 for the 4th pin
    const uint32_t mask = ~(0b111<<tree_bits_position_in_register);
    // Apply mask
    _gpio[pin / pins_per_register] &= mask;
}

void CRPI_GPIO::setGPIOModeOut(int pin)
{
    // Each register can contain 10 pins
    const uint8_t pins_per_register = 10;
    // Calculates the position of the 3 bit mask in the 32 bits register
    const uint8_t tree_bits_position_in_register = (pin%pins_per_register)*3;
    // Create a mask to enable the bit that sets output functionality
    // 0b00'000'000'000'000'000'000'001'000'000'000 enables output for the 4th pin
    const uint32_t mask_with_bit = 0b001 << tree_bits_position_in_register;
    const uint32_t mask = 0b111 << tree_bits_position_in_register;
    // Clear all bits in our position and apply our mask with alt values
    uint32_t register_value = _gpio[pin / pins_per_register];
    register_value &= ~mask;
    _gpio[pin / pins_per_register] = register_value | mask_with_bit;
}

void CRPI_GPIO::setGPIOHigh(int pin)
{
    // Calculate index of the array for the register GPSET0 (0x7E20'001C)
    constexpr uint32_t gpset0_memory_offset_value = 0x1c;
    constexpr uint32_t gpset0_index_value = gpset0_memory_offset_value / sizeof(*_gpio);
    _gpio[gpset0_index_value] = 1 << pin;
}

void CRPI_GPIO::setGPIOLow(int pin)
{
    // Calculate index of the array for the register GPCLR0 (0x7E20'0028)
    constexpr uint32_t gpclr0_memory_offset_value = 0x28;
    constexpr uint32_t gpclr0_index_value = gpclr0_memory_offset_value / sizeof(*_gpio);
    _gpio[gpclr0_index_value] = 1 << pin;
}

bool CRPI_GPIO::getGPIOLogicState(int pin)
{
    // Calculate index of the array for the register GPLEV0 (0x7E20'0034)
    constexpr uint32_t gplev0_memory_offset_value = 0x34;
    constexpr uint32_t gplev0_index_value = gplev0_memory_offset_value / sizeof(*_gpio);
    return _gpio[gplev0_index_value] & (1 << pin);
}

uint8_t CRPI_GPIO::read (uint8_t pin)
{
    if (pin >= GPIO_RPI_MAX_NUMBER_PINS) {
        return 0;
    }
    return static_cast<uint8_t>(getGPIOLogicState(pin));
}

void CRPI_GPIO::write(uint8_t pin, uint8_t value)
{
    if (value != 0) {
        setGPIOHigh(pin);
    } else {
        setGPIOLow(pin);
    }
}

void CRPI_GPIO::toggle(uint8_t pin)
{
    if (pin >= GPIO_RPI_MAX_NUMBER_PINS) {
        return ;
    }
    uint32_t flag = (1 << pin);
    _gpio_output_port_status ^= flag;
    write(pin, (_gpio_output_port_status & flag) >> pin);
}


uint32_t CRPI_GPIO::getAddress(CRPI_GPIO::Address address, CRPI_GPIO::PeripheralOffset offset) const
{
    return static_cast<uint32_t>(address) + static_cast<uint32_t>(offset);
}   