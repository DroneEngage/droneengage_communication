
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <fstream>
#include <memory>
#include <cctype>
#include "./helpers/colors.hpp"
#include "./helpers/helpers.hpp"

#include "localConfigFile.hpp"

using namespace de;


const Json_de& CLocalConfigFile::GetConfigJSON()
{
    return  m_ConfigJSON;
}


void CLocalConfigFile::initConfigFile (const char* fileURL)
{
    m_ConfigJSON={};
    m_parseFailed = false;
    m_fileContents.str("");

    m_fileURL = std::string(fileURL);

    ReadFile (fileURL);

    ParseData (m_fileContents.str());

#ifdef DEBUG
    std::cout << _LOG_CONSOLE_TEXT << "DEBUG localConfig: parsed JSON has " << m_ConfigJSON.size() << " key(s)" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    if (m_ConfigJSON.contains("party_id"))
        std::cout << _LOG_CONSOLE_TEXT << "DEBUG localConfig: party_id=" << m_ConfigJSON["party_id"] << _NORMAL_CONSOLE_TEXT_ << std::endl;
    if (m_ConfigJSON.contains("auth_verify_ssl"))
        std::cout << _LOG_CONSOLE_TEXT << "DEBUG localConfig: auth_verify_ssl=" << m_ConfigJSON["auth_verify_ssl"] << _NORMAL_CONSOLE_TEXT_ << std::endl;
#endif
}


void CLocalConfigFile::apply()
{
    if (m_parseFailed)
    {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "apply() SKIPPED: local config file failed to parse — refusing to overwrite the on-disk file." << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return;
    }
    WriteFile (m_fileURL.c_str());
}

void CLocalConfigFile::clearFile()
{
    m_ConfigJSON={};
    WriteFile (m_fileURL.c_str());
}



void CLocalConfigFile::WriteFile (const char * fileURL)
{
    std::ofstream stream;
    std::cout << _LOG_CONSOLE_BOLD_TEXT << "Write internal config file: " << _SUCCESS_CONSOLE_TEXT_ << fileURL << _NORMAL_CONSOLE_TEXT_ << " ...." ;

    stream.open (fileURL , std::ifstream::out | std::ios::trunc );
    if (!stream) {
        std::cout << _ERROR_CONSOLE_TEXT_ << " FAILED " << _NORMAL_CONSOLE_TEXT_ << std::endl;
        exit(1); // terminate with error
    }

    std::string j = m_ConfigJSON.dump(4);
    stream << j;
    stream.close();
    std::cout << _SUCCESS_CONSOLE_TEXT_ << " succeeded "  << _NORMAL_CONSOLE_TEXT_ << std::endl;

    return ;
}

void CLocalConfigFile::ReadFile (const char * fileURL)
{
    std::ifstream stream;
    std::cout << _LOG_CONSOLE_TEXT << "Read internal config file: " << _SUCCESS_CONSOLE_TEXT_ << fileURL << _NORMAL_CONSOLE_TEXT_ << " ...." ;

    stream.open (fileURL , std::ifstream::in);
    if (!stream) {
        std::cout << _INFO_CONSOLE_TEXT << " trying to create one " << _NORMAL_CONSOLE_TEXT_ << std::endl;
        WriteFile (fileURL);
        // put JSON as string to keep contents consistent.
        m_fileContents << m_ConfigJSON;
        return ;
    }

    m_fileContents <<  stream.rdbuf();

    std::cout << _SUCCESS_CONSOLE_TEXT_ << " succeeded "  << _NORMAL_CONSOLE_TEXT_ << std::endl;

    return ;
}


bool CLocalConfigFile::ParseData (std::string jsonString)
{
    m_parseFailed = false;

   try
   {
        std::string cleaned = removeComments(jsonString);

        // Remove trailing commas before } or ] (tolerant of human-edited JSON)
        std::string tolerant;
        tolerant.reserve(cleaned.size());
        for (size_t i = 0; i < cleaned.size(); ++i)
        {
            if (cleaned[i] == ',')
            {
                size_t j = i + 1;
                while (j < cleaned.size() && std::isspace(static_cast<unsigned char>(cleaned[j]))) ++j;
                if (j < cleaned.size() && (cleaned[j] == '}' || cleaned[j] == ']'))
                {
                    continue;
                }
            }
            tolerant += cleaned[i];
        }

        m_ConfigJSON = Json_de::parse(tolerant);
   }
   catch(const std::exception& e)
   {
    std::cerr << e.what() << '\n';
    m_parseFailed = true;
    return false;
   }

   return true;

}


void CLocalConfigFile::addStringField(const char * field, const char * value)
{
    m_ConfigJSON[std::string(field)] = std::string(value);
}

void CLocalConfigFile::ModifyStringField(const char* field, const char* newValue)
{
    std::string key = std::string(field);
    std::string value = std::string(newValue);

    if (m_ConfigJSON.contains(key))
    {
        m_ConfigJSON[key] = value;
    }
    else
    {
        m_ConfigJSON[key] = value;
    }
}

std::string CLocalConfigFile::getStringField(const char * field) const
{
    if (!m_ConfigJSON.contains(std::string(field))) return {};

    return m_ConfigJSON[std::string(field)].get<std::string>();
}


void CLocalConfigFile::addNumericField(const char * field, const u_int32_t & value)
{
    m_ConfigJSON[std::string(field)] = value;
}


const u_int32_t CLocalConfigFile::getNumericField(const char * field) const
{
    if (!m_ConfigJSON.contains(std::string(field))) return 0xffffffff;

    return m_ConfigJSON[std::string(field)].get<int>();
}

void CLocalConfigFile::ModifyNumericField(const char* field, const u_int32_t& newValue)
{
    std::string key = std::string(field);
    m_ConfigJSON[key] = newValue;
}


void CLocalConfigFile::removeFieldByName(const char* fieldName)
{
    std::string key = std::string(fieldName);
    if (m_ConfigJSON.contains(key))
    {
        m_ConfigJSON.erase(key);
    }
}


void CLocalConfigFile::addDoubleField(const char * field, double value)
{
    m_ConfigJSON[std::string(field)] = value;
}

double CLocalConfigFile::getDoubleField(const char * field) const
{
    if (!m_ConfigJSON.contains(std::string(field))) return 0.0;

    return m_ConfigJSON[std::string(field)].get<double>();
}

void CLocalConfigFile::ModifyDoubleField(const char* field, double newValue)
{
    std::string key = std::string(field);
    m_ConfigJSON[key] = newValue;
}
