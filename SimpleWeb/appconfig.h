#pragma once
#include <unordered_map>
#include <string>
#include <mutex>
#include <optional>

class Configuration
{
public:
    static void Set(const std::string& key, const std::string& value);
    static std::string Get(const std::string& key, const std::string& defaultValue = "");
    static int GetInt(const std::string& key, int defaultValue = 0);
    static bool GetBool(const std::string& key, bool defaultValue = false);
    static double GetDouble(const std::string& key, double defaultValue = 0.0);
    static bool Contains(const std::string& key);
    static bool LoadFromFile(const std::string& filePath);
private:
    static std::unordered_map<std::string, std::string> settings;
    static std::mutex settingsMutex;
};
