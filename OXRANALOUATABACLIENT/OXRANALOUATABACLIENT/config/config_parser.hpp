#pragma once
#include <string>
#include <map>

class SimpleJsonParser
{
public:
    static bool LoadOffsets(const std::string& jsonPath, std::map<std::string, uint32_t>& offsets);
    static bool LoadButtons(const std::string& jsonPath, std::map<std::string, uint32_t>& buttons);
    static bool LoadSchemaOffsets(const std::string& jsonPath, const std::string& className, std::map<std::string, uint32_t>& offsets);
};
