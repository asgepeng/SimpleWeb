#include "logger.h"

void Web::Logger::Info(const std::string& msg)
{
	Log("[INFO] ", msg);
}

void Web::Logger::Error(const std::string& msg)
{
	Log("[WARN] ", msg);
}

void Web::Logger::Warn(const std::string& msg)
{
	Log("[ERROR] ", msg);
}
