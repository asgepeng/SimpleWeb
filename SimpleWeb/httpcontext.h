#pragma once
#include "response.h"

namespace Web
{
    class HttpContext {
    public:
        HttpContext(const HttpRequest& req) : request(req) { }
        const HttpRequest& request;
        HttpResponse response;

        std::string GetParam(const std::string& key) const;
        std::unordered_map<std::string, std::string> routeData;
    };
}
