
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <fstream>
#include <memory>
#include <iomanip>
#include <ctime>
#include <vector>
#include "../helpers/colors.hpp"
#include "../helpers/helpers.hpp"

#include "configFile.hpp"

using namespace de;

const Json_de& CConfigFile::GetConfigJSON()
{
    return m_ConfigJSON;
}

void CConfigFile::initConfigFile (const char* fileURL)
{
    m_file_url = std::string(fileURL);
    CConfigFile::ReadFile (m_file_url.c_str());
    CConfigFile::ParseData (m_fileContents.str());
    m_BaseJSON = m_ConfigJSON;   // pristine published config kept for saves
#ifndef DE_DISABLE_TRY
    try {
#endif
        m_lastWriteTime = std::filesystem::last_write_time(m_file_url.c_str());
        std::cout << _INFO_CONSOLE_TEXT << "Initial last write time obtained." << _NORMAL_CONSOLE_TEXT_ << std::endl;
#ifndef DE_DISABLE_TRY
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << _ERROR_CONSOLE_BOLD_TEXT_ << "Error: Could not get initial file write time for '" << m_file_url << "': " << e.what() << _NORMAL_CONSOLE_TEXT_ << std::endl;
    }
#endif
}

void CConfigFile::reloadFile ()
{
    m_updatePending = false;
    CConfigFile::ReadFile (m_file_url.c_str());
    CConfigFile::ParseData (m_fileContents.str());
    m_BaseJSON = m_ConfigJSON;   // published file changed on disk -> reset base
    // Re-apply local overrides on top of the freshly loaded base so device
    // specific settings survive an external edit of the published config.
    if (!m_LocalOverrides.is_null())
    {
        applyLocalOverrides(m_LocalOverrides);
    }

#ifndef DE_DISABLE_TRY
    try {
#endif
        m_lastWriteTime = std::filesystem::last_write_time(m_file_url.c_str());
#ifndef DE_DISABLE_TRY
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << _ERROR_CONSOLE_BOLD_TEXT_ << "Error: Could not update file write time after reload for '" << m_file_url << "': " << e.what() << _NORMAL_CONSOLE_TEXT_ << std::endl;
    }
#endif
}

bool CConfigFile::fileUpdated ()
{
    // Check if update was called since last check
    if (m_updatePending) {
        m_updatePending = false;
        std::cout << _INFO_CONSOLE_TEXT << "Config file updated via updateJSON." << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return true;
    }

    if (m_file_url.empty()) {
        std::cerr << _ERROR_CONSOLE_BOLD_TEXT_ << "Error: File URL is empty." << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return false;
    }

    if (!std::filesystem::exists(m_file_url)) {
        std::cerr << _ERROR_CONSOLE_BOLD_TEXT_ << "Error: Config file does not exist: " << m_file_url << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return false;
    }
#ifndef DE_DISABLE_TRY
    try {
#endif
        std::filesystem::file_time_type lastWriteTime = std::filesystem::last_write_time(m_file_url.c_str());
        if (lastWriteTime == m_lastWriteTime) return false;

        m_lastWriteTime = lastWriteTime;
        std::cout << _INFO_CONSOLE_TEXT << "Initial last write time obtained." << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return true;
#ifndef DE_DISABLE_TRY
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << _ERROR_CONSOLE_BOLD_TEXT_ << "Error: Could not get file write time for '" << m_file_url << "': " << e.what() << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return false;
    }
#endif
}

void CConfigFile::ReadFile (const char * fileURL)
{
    std::ifstream stream;
    std::cout << _LOG_CONSOLE_BOLD_TEXT << "Read config file: " << _INFO_CONSOLE_TEXT << fileURL << "\033[0m ...." ;

    stream.open (fileURL , std::ifstream::in);
    if (!stream) {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "FATAL ERROR:" << _ERROR_CONSOLE_TEXT_ << " FAILED to read config file " << _NORMAL_CONSOLE_TEXT_ << std::endl;
        exit(1); // terminate with error
    }

    m_fileContents.str("");
    m_fileContents << stream.rdbuf();
    std::cout << _SUCCESS_CONSOLE_TEXT_ << " succeeded " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    return;
}

void CConfigFile::ParseData (std::string jsonString)
{
    m_ConfigJSON = Json_de::parse(removeComments(jsonString));
    std::cout << _SUCCESS_CONSOLE_TEXT_ << " config file parsed successfully " << _NORMAL_CONSOLE_TEXT_ << std::endl;
}

// Apply a flat or dotted-key update JSON onto `target`. Used to mirror UI
// edits into both the runtime config (m_ConfigJSON) and the pristine base
// (m_BaseJSON) so saves persist edits without leaking local overrides.
static void applyUpdateJSON(Json_de& target, const Json_de& updateJson)
{
    for (const auto& item : updateJson.items()) {
        std::string key = item.key();

        // Check if key contains '.' for nested structure (e.g., "follow_me.quad.PID_P_X")
        if (key.find('.') != std::string::npos) {
            // Split key by '.' to handle nested structures
            std::vector<std::string> pathParts;
            std::stringstream ss(key);
            std::string part;
            while (std::getline(ss, part, '.')) {
                if (!part.empty()) {
                    pathParts.push_back(part);
                }
            }

            if (pathParts.empty()) {
                std::cerr << _ERROR_CONSOLE_BOLD_TEXT_ << "Error: Invalid key format: " << key << _NORMAL_CONSOLE_TEXT_ << std::endl;
                continue;
            }

            // Navigate/create nested structure
            Json_de* currentNode = &target;
            for (size_t i = 0; i < pathParts.size() - 1; ++i) {
                if (!currentNode->contains(pathParts[i])) {
                    (*currentNode)[pathParts[i]] = Json_de::object();
                }
                currentNode = &((*currentNode)[pathParts[i]]);
            }

            // Update the value at the nested location using the last part as the key
            (*currentNode)[pathParts.back()] = item.value();
            std::cout << _INFO_CONSOLE_TEXT << "Updated/Added nested JSON key: " << key << _NORMAL_CONSOLE_TEXT_ << std::endl;
        } else {
            // Update existing entry or add new entry at root level
            target[key] = item.value();
            std::cout << _INFO_CONSOLE_TEXT << "Updated/Added JSON key: " << key << _NORMAL_CONSOLE_TEXT_ << std::endl;
        }
    }
}

void CConfigFile::updateJSON(const std::string& jsonString)
{
#ifndef DE_DISABLE_TRY
    try {
#endif
        m_updatePending = true;
        Json_de updateJson = Json_de::parse(removeComments(jsonString));
        // Mirror UI edits into both the runtime config and the pristine base
        // so saveConfigFile() persists them into the published file without
        // dragging in any local overrides.
        applyUpdateJSON(m_ConfigJSON, updateJson);
        applyUpdateJSON(m_BaseJSON, updateJson);
        saveConfigFile();
#ifndef DE_DISABLE_TRY
    } catch (const std::exception& e) {
        std::cerr << _ERROR_CONSOLE_BOLD_TEXT_ << "Error: Failed to parse update JSON: " << e.what() << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return;
    }
#endif
}

// Recursively merge `overrides` on top of `target`. Nested objects are
// merged key-by-key; non-object values replace the target outright.
static void deepMerge(Json_de& target, const Json_de& overrides)
{
    if (!overrides.is_object()) {
        target = overrides;
        return;
    }
    if (!target.is_object()) {
        target = Json_de::object();
    }
    for (const auto& item : overrides.items()) {
        const std::string& key = item.key();
        if (target.contains(key) && target[key].is_object() && item.value().is_object()) {
            deepMerge(target[key], item.value());
        } else {
            target[key] = item.value();
        }
    }
}

void CConfigFile::applyLocalOverrides(const Json_de& localJson)
{
    m_LocalOverrides = localJson;
    if (localJson.is_null() || localJson.empty()) return;
    // Reset runtime to the pristine base, then layer overrides on top so
    // calling this repeatedly (e.g. after reloadFile) does not stack.
    m_ConfigJSON = m_BaseJSON;
    deepMerge(m_ConfigJSON, localJson);
    std::cout << _INFO_CONSOLE_TEXT << "Applied local config overrides (" << localJson.size() << " top-level key(s))." << _NORMAL_CONSOLE_TEXT_ << std::endl;
}

void CConfigFile::saveConfigFile()
{
    // Create backup if file exists
    if (std::filesystem::exists(m_file_url)) {
        // Generate timestamp for backup
        auto now = std::time(nullptr);
        std::stringstream timestamp;
        timestamp << std::put_time(std::localtime(&now), "%Y%m%d_%H%M%S");
        std::string backup_url = m_file_url + ".bak_" + timestamp.str();

#ifndef DE_DISABLE_TRY
        try {
#endif
            std::filesystem::copy_file(m_file_url, backup_url, std::filesystem::copy_options::overwrite_existing);
            std::cout << _INFO_CONSOLE_TEXT << "Backup created: " << backup_url << _NORMAL_CONSOLE_TEXT_ << std::endl;
#ifndef DE_DISABLE_TRY
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << _ERROR_CONSOLE_BOLD_TEXT_ << "Error: Could not create backup file '" << backup_url << "': " << e.what() << _NORMAL_CONSOLE_TEXT_ << std::endl;
        }
#endif
    }

    std::ofstream outFile(m_file_url);
    if (!outFile) {
        std::cerr << _ERROR_CONSOLE_BOLD_TEXT_ << "Error: Could not open config file for writing: " << m_file_url << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return;
    }
    // Persist the pristine base (no local overrides) so the published
    // config file stays clean and shareable.
    outFile << m_BaseJSON.dump(4);
    outFile.close();
    std::cout << _SUCCESS_CONSOLE_TEXT_ << "Config file saved successfully: " << m_file_url << _NORMAL_CONSOLE_TEXT_ << std::endl;

#ifndef DE_DISABLE_TRY
    try {
#endif
        m_lastWriteTime = std::filesystem::last_write_time(m_file_url.c_str());
#ifndef DE_DISABLE_TRY
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << _ERROR_CONSOLE_BOLD_TEXT_ << "Error: Could not update file write time after save for '" << m_file_url << "': " << e.what() << _NORMAL_CONSOLE_TEXT_ << std::endl;
    }
#endif
}
