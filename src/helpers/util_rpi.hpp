#ifndef CURIL_RPI_H
#define CURIL_RPI_H


#include <iostream>
#include <cstdint>


namespace helpers
{
    class CUtil_Rpi
    {
        public:
            //https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
            static CUtil_Rpi& getInstance()
            {
                static helpers::CUtil_Rpi instance;

                return instance;
            }

            CUtil_Rpi(helpers::CUtil_Rpi const&)           = delete;
            void operator=(helpers::CUtil_Rpi const&)      = delete;

        private:

            CUtil_Rpi();


        public:
            
            ~CUtil_Rpi (){};
           
        public:
            int get_rpi_model ()  const;
            bool get_cpu_serial (std::string &cpu_serial)  const;

            bool get_cpu_temprature(uint32_t &cpu_temprature) const;
            bool get_throttled (uint32_t &cpu_serial) const ;

        protected:
            // Called in the constructor once
            int _check_rpi_version();
            int _check_rpi_version_by_rev();


        private:
            int _rpi_version = -1;
            bool _not_found = true;
    };
}

#endif