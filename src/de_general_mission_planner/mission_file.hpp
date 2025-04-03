// MissionFile.h

#ifndef MISSION_FILE_H_
#define MISSION_FILE_H_

#include <string>

namespace de {
namespace mission {

class CMissionFile {
public:
    static CMissionFile& getInstance() {
        static CMissionFile instance;
        return instance;
    }

    CMissionFile(CMissionFile const&) = delete;
    void operator=(CMissionFile const&) = delete;

protected:
    CMissionFile() {}

public:
    ~CMissionFile() {}

public:
    std::string readMissionFile(const std::string& file_name);
    void writeMissionFile(const std::string& file_name, const std::string& file_content);
    void deleteMissionFile(const std::string& file_name);
};

} // namespace mission
} // namespace de

#endif // MISSION_FILE_H_