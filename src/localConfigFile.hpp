#ifndef CLOCALCONFIGFILE_H

#define CLOCALCONFIGFILE_H

#include <sstream>
#include "./helpers/json_nlohmann.hpp"
using Json_de = nlohmann::json;

namespace de
{
    class CLocalConfigFile 
    {

        public:
            //https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
            static CLocalConfigFile& getInstance()
            {
                static CLocalConfigFile    instance; // Guaranteed to be destroyed.
                                                // Instantiated on first use.
                return instance;
            }
            CLocalConfigFile(CLocalConfigFile const&)        = delete;
            void operator=(CLocalConfigFile const&)          = delete;

            // Note: Scott Meyers mentions in his Effective Modern
            //       C++ book, that deleted functions should generally
            //       be public as it results in better error messages
            //       due to the compilers behavior to check accessibility
            //       before deleted status
        private:
            CLocalConfigFile() {}                    // Constructor? (the {} brackets) are needed here.

            // C++ 11
            // =======
            // We can use the better technique of deleting the methods
            // we don't want.
            

        public:
            void initConfigFile (const char* fileURL);
            const Json_de& GetConfigJSON();
            void clearFile();
            void apply();
            
            std::string getStringField(const char * field) const;
            void addStringField(const char * field, const char * value);
            
            const u_int32_t getNumericField(const char * field) const ;
            void addNumericField(const char * field, const u_int32_t & value);

        protected:
            void ReadFile (const char * fileURL);
            void WriteFile (const char * fileURL);
            bool ParseData (std::string jsonString);
            

        private:
            std::string m_fileURL;
            std::stringstream m_fileContents;
            Json_de m_ConfigJSON;
        

    };
}

#endif
