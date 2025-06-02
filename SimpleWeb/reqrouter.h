#pragma once
#include <string>

namespace Web
{
    class RequestRouter {
    public:
        std::string RouteRequest(const std::string& request);
    };
}
