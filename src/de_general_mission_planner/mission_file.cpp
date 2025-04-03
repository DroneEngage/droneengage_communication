#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem> // For C++17 and later
#include <iostream>

#include "../helpers/colors.hpp"

#include "mission_file.hpp"

namespace de {
namespace mission {

std::string CMissionFile::readMissionFile(const std::string& file_name) {
    std::ifstream file(file_name);
    if (!file.is_open()) {
        
        std::cout << std::endl << _ERROR_CONSOLE_BOLD_TEXT_ "Could not open file: " << _BOLD_CONSOLE_TEXT_ << file_name << _NORMAL_CONSOLE_TEXT_ << std::endl;
    
        return std::string();
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void CMissionFile::writeMissionFile(const std::string& file_name, const std::string& file_content) {

    std::ofstream file(file_name, std::ios::trunc); // Open and truncate

    if (!file.is_open()) {
        std::cout << std::endl << _ERROR_CONSOLE_BOLD_TEXT_ "Could not open file: " << _BOLD_CONSOLE_TEXT_ << file_name << _NORMAL_CONSOLE_TEXT_ << std::endl;

        throw std::runtime_error("Could not open file for writing: " + file_name);
    }

    file << file_content;
}

void CMissionFile::deleteMissionFile(const std::string& file_name) {
    std::filesystem::remove(file_name);
}

} // namespace mission
} // namespace de