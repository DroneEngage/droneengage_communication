
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <fstream>
#include <memory> 
#include "./helpers/colors.hpp"
#include "./helpers/helpers.hpp"

#include "configFile.hpp"

using namespace uavos;


const Json& CConfigFile::GetConfigJSON()
{
    return  m_ConfigJSON;
}


void CConfigFile::InitConfigFile (const char* fileURL)
{
    CConfigFile::ReadFile (fileURL);
    
    CConfigFile::ParseData (m_fileContents.str());
}


void CConfigFile::ReadFile (const char * fileURL)
{
    std::ifstream stream;
    std::cout << _LOG_CONSOLE_TEXT << "Read config file: " << _SUCCESS_CONSOLE_TEXT_ << fileURL << _NORMAL_CONSOLE_TEXT_ << " ...." ;

    stream.open (fileURL , std::ifstream::in);
    if (!stream) {
        std::cout << _ERROR_CONSOLE_TEXT_ << " FAILED " << _NORMAL_CONSOLE_TEXT_ << std::endl;
        exit(1); // terminate with error
    }
    
    std::cout << _SUCCESS_CONSOLE_TEXT_ << " succeeded "  << _NORMAL_CONSOLE_TEXT_ << std::endl;

    m_fileContents <<  stream.rdbuf();
    return ;
}


void CConfigFile::ParseData (std::string jsonString)
{
    m_ConfigJSON = Json::parse(removeComments(jsonString));
}