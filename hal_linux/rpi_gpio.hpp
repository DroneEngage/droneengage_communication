#ifndef HAL_LINUX_RPI_GPIO_H_
#define HAL_LINUX_RPI_GPIO_H_

#include <string.h>
#include "../hal/gpio.hpp"


namespace hal_linux
{


class CRPI_GPIO :  public hal::CGPIO
{
    public:
        static CRPI_GPIO& getInstance()
        {
            static CRPI_GPIO instance;

            return instance;
        }


        CRPI_GPIO (CRPI_GPIO const &)           = delete;
        void operator=(CRPI_GPIO const &)   = delete;
    
    protected:
        CRPI_GPIO();

    public:
        ~CRPI_GPIO(){};

    public:
        
        void pinMode (uint8_t pin, uint8_t output) override ;
        uint8_t read (uint8_t pin) override ;
        void write(uint8_t pin, uint8_t value) override ;
        uint8_t toggle(uint8_t pin) override ;
        bool init () override;
    
    private:
    

    /**
     * @brief Open memory device to allow gpio address access
     *  Should be used before get_memory_pointer calls in the initialization
     *
     * @return true
     * @return false
     */
    bool openMemoryDevice();

    /**
     * @brief Close open memory device
     *
     */
    void closeMemoryDevice();


    // Raspberry Pi BASE memory address
    enum class Address : uint32_t {
        BCM2708_PERIPHERAL_BASE = 0x20000000, // Raspberry Pi 0/1
        BCM2709_PERIPHERAL_BASE = 0x3F000000, // Raspberry Pi 2/3
        BCM2711_PERIPHERAL_BASE = 0xFE000000, // Raspberry Pi 4
    };

    // Offset between peripheral base address
    enum class PeripheralOffset : uint32_t {
        GPIO = 0x200000,
    };

    
    /**
     * @brief Set a specific GPIO as input
     * Check Linux::GPIO_RPI::set_gpio_mode_alt for more information.
     *
     * @param pin
     */
    void setGPIOModeIn(int pin);
    
    /**
     * @brief Set a specific GPIO as output
     * Check Linux::GPIO_RPI::set_gpio_mode_alt for more information.
     *
     * @param pin
     */
    void setGPIOModeOut(int pin);
    
    /**
     * @brief Modify GPSET0 (GPIO Pin Output Set 0) register to set pin as high
     * Writing zero to this register has no effect, please use Linux::GPIO_RPI::set_gpio_low
     * to set pin as low.
     *
     * GPSET0 is a 32bits register that each bit points to a respective GPIO pin:
     * 0b...101
     *      ││└── GPIO Pin 1, 1st bit, LSBit, defined as High
     *      │└─── GPIO Pin 2, 2nd bit, No effect
     *      └──── GPIO Pin 3, 3rd bit, defined as High
     *     ...
     *
     * @param pin
     */
    void setGPIOHigh(int pin);
    
    /**
     * @brief Modify GPCLR0 (GPIO Pin Output Clear 0) register to set pin as low
     * Writing zero to this register has no effect, please use Linux::GPIO_RPI::set_gpio_high
     * to set pin as high.
     *
     * GPCLR0 is a 32bits register that each bit points to a respective GPIO pin:
     * 0b...101
     *      ││└── GPIO Pin 1, 1st bit, LSBit, defined as Low
     *      │└─── GPIO Pin 2, 2nd bit, No effect
     *      └──── GPIO Pin 3, 3rd bit, defined as Low
     *
     * @param pin
     */
    void setGPIOLow(int pin);
    
    /**
     * @brief Read GPLEV0 (GPIO Pin Level 0) register check the logic state of a specific pin
     *
     * GPLEV0 is a 32bits register that each bit points to a respective GPIO pin:
     * 0b...101
     *      ││└── GPIO Pin 1, 1st bit, LSBit, Pin is in High state
     *      │└─── GPIO Pin 2, 2nd bit, Pin is in Low state
     *      └──── GPIO Pin 3, 3rd bit, Pin is in High state
     *
     * @param pin
     * @return true
     * @return false
     */
    bool getGPIOLogicState(int pin);
    

    uint32_t getAddress(CRPI_GPIO::Address address, CRPI_GPIO::PeripheralOffset offset) const;
    volatile uint32_t* get_memory_pointer(uint32_t address, uint32_t range) const;



    // Memory pointer to gpio registers
    volatile uint32_t* m_gpio;
    // Memory range for the gpio registers
    static const uint8_t m_gpio_registers_memory_range;
    // File descriptor for the memory device file
    // If it's negative, then there was an error opening the file.
    int m_system_memory_device;
    // store GPIO output status.
    uint32_t m_gpio_output_port_status = 0x00;

    // Path to memory device (E.g: /dev/mem)
    const std::string m_system_memory_device_path = "/dev/mem";
    
    bool m_initialized = false;
};
};

#endif