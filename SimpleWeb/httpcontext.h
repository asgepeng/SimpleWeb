#pragma once
#include "response.h"

namespace Web
{
    class HttpContext {
    public:
        const HttpRequest& Request;
        HttpResponse Response;
        std::unordered_map<std::string, std::string> RouteData;

        HttpContext(const HttpRequest& req) : Request(req), Response(req.Socket) {}

        std::string GetParam(const std::string& key) const 
        {
            auto it = RouteData.find(key);
            return it != RouteData.end() ? it->second : "";
        }
    };
}
