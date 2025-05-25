#include "appconfig.h"
#include <fstream>
#include <sstream>
#include <cstdlib>

std::unordered_map<std::string, std::string> Configuration::settings;
std::mutex Configuration::settingsMutex;

void Configuration::Set(const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> lock(settingsMutex);
    settings[key] = value;
}

std::string Configuration::Get(const std::string& key, const std::string& defaultValue)
{
    std::lock_guard<std::mutex> lock(settingsMutex);
    auto it = settings.find(key);
    return (it != settings.end()) ? it->second : defaultValue;
}

int Configuration::GetInt(const std::string& key, int defaultValue)
{
    std::string value = Get(key);
    try {
        return std::stoi(value);
    }
    catch (...) {
        return defaultValue;
    }
}

bool Configuration::GetBool(const std::string& key, bool defaultValue)
{
    std::string value = Get(key);
    if (value == "true" || value == "1") return true;
    if (value == "false" || value == "0") return false;
    return defaultValue;
}

double Configuration::GetDouble(const std::string& key, double defaultValue)
{
    std::string value = Get(key);
    try {
        return std::stod(value);
    }
    catch (...) {
        return defaultValue;
    }
}

bool Configuration::Contains(const std::string& key)
{
    std::lock_guard<std::mutex> lock(settingsMutex);
    return settings.find(key) != settings.end();
}

bool Configuration::LoadFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#') continue;

        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos)
        {
            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);
            Set(key, value);
        }
    }
    return true;
}
