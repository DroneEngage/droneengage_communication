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

const uint8_t CRPI_GPIO::m_gpio_registers_memory_range = 0xB4;

CRPI_GPIO::CRPI_GPIO()
{
    
}




bool CRPI_GPIO::openMemoryDevice()
{
    m_system_memory_device = open(m_system_memory_device_path.c_str(), O_RDWR|O_SYNC|O_CLOEXEC);
    if (m_system_memory_device < 0) {
        return false;
    }

    return true;
}

void CRPI_GPIO::closeMemoryDevice()
{
    close(m_system_memory_device);
    // Invalidate device variable
    m_system_memory_device = -1;
}


bool CRPI_GPIO::init()
{
    // do not initialize
    // get_memory_pointer may return another address and may invalidate the current one.
    if (m_initialized) return true;
    
    const int rpi_version = helpers::CUtil_Rpi::getInstance().get_rpi_model();
    if (rpi_version == -1) 
    {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Error: Cannot initialize GPIO because it is not RPI-Board" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return false;
    }
    
    CRPI_GPIO::Address peripheral_base;
    
    if(rpi_version == 1) {
        peripheral_base = Address::BCM2708_PERIPHERAL_BASE;
    } else if (rpi_version == 2) {
        peripheral_base = Address::BCM2709_PERIPHERAL_BASE;
    } else {
        peripheral_base = Address::BCM2711_PERIPHERAL_BASE;
    }

    if (!openMemoryDevice()) {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Error: Failed to initialize memory device." << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return false;
    }

    const uint32_t gpio_address = getAddress(peripheral_base, PeripheralOffset::GPIO);

    m_gpio = get_memory_pointer(gpio_address, CRPI_GPIO::m_gpio_registers_memory_range);
    if (!m_gpio) {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Error: Failed to get GPIO memory map." << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return false;
    }

    // No need to keep mem_fd open after mmap
    closeMemoryDevice();

    m_initialized = true;

    return true;
}

volatile uint32_t* CRPI_GPIO::get_memory_pointer(uint32_t address, uint32_t range) const
{
    auto pointer = mmap(
        nullptr,                         // Any adddress in our space will do
        range,                           // Map length
        PROT_READ|PROT_WRITE|PROT_EXEC,  // Enable reading & writing to mapped memory
        MAP_SHARED|MAP_LOCKED,           // Shared with other processes
        m_system_memory_device,           // File to map
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
    m_gpio[pin / pins_per_register] &= mask;
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
    uint32_t register_value = m_gpio[pin / pins_per_register];
    register_value &= ~mask;
    m_gpio[pin / pins_per_register] = register_value | mask_with_bit;
}

void CRPI_GPIO::setGPIOHigh(int pin)
{
    // Calculate index of the array for the register GPSET0 (0x7E20'001C)
    constexpr uint32_t gpset0_memory_offset_value = 0x1c;
    constexpr uint32_t gpset0_index_value = gpset0_memory_offset_value / sizeof(*m_gpio);
    m_gpio[gpset0_index_value] = 1 << pin;
}

void CRPI_GPIO::setGPIOLow(int pin)
{
    // Calculate index of the array for the register GPCLR0 (0x7E20'0028)
    constexpr uint32_t gpclr0_memory_offset_value = 0x28;
    constexpr uint32_t gpclr0_index_value = gpclr0_memory_offset_value / sizeof(*m_gpio);
    m_gpio[gpclr0_index_value] = 1 << pin;
}

bool CRPI_GPIO::getGPIOLogicState(int pin)
{
    // Calculate index of the array for the register GPLEV0 (0x7E20'0034)
    constexpr uint32_t gplev0_memory_offset_value = 0x34;
    constexpr uint32_t gplev0_index_value = gplev0_memory_offset_value / sizeof(*m_gpio);
    return m_gpio[gplev0_index_value] & (1 << pin);
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

uint8_t CRPI_GPIO::toggle(uint8_t pin)
{
    if (pin >= GPIO_RPI_MAX_NUMBER_PINS) {
        return -1; //255 as uint8_t is +
    }
    const uint32_t flag = (1 << pin);
    m_gpio_output_port_status ^= flag;
    const uint8_t status = (m_gpio_output_port_status & flag) >> pin;
    write(pin, status);

    return status;
}


uint32_t CRPI_GPIO::getAddress(CRPI_GPIO::Address address, CRPI_GPIO::PeripheralOffset offset) const
{
    return static_cast<uint32_t>(address) + static_cast<uint32_t>(offset);
}   