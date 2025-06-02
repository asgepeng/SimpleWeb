#pragma once
#include <string>
#include <iostream>
#include <mutex>

namespace Web
{
    class Logger 
    {
    public:
        void Info(const std::string& msg);
        void Error(const std::string& msg);
        void Warn(const std::string& msg);
    private:
        void Log(const std::string& level, const std::string& msg)
        {
            std::lock_guard<std::mutex> lock(mutex);
            std::cout << level << msg << std::endl;
        }

        std::mutex mutex;
    };
}
