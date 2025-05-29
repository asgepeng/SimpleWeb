#pragma once
#include "httpcontext.h"
#include <functional>

namespace Web
{
    class Router;
    class RouteConfig
    {
    public:
        virtual void RegisterEndPoints(Router* router) = 0;
        virtual ~RouteConfig() = default;
    };

    class RoutePattern
    {
    public:
        explicit RoutePattern(const std::string& pattern);
        bool match(const std::string& path, std::unordered_map<std::string, std::string>& routeValues) const;

    private:
        std::vector<std::string> segments_;
        std::vector<bool> isParam_;
    };

    class Router
    {
    public:
        //using Handler = std::function<std::string(Web::HttpContext&)>;
        using Handler = std::function<void(Web::HttpContext&)>;

        void HandleRequest(Web::HttpContext& context);
        void MapGet(const std::string& pattern, Handler handler);
        void MapPost(const std::string& pattern, Handler handler);
        void MapPut(const std::string& pattern, Handler handler);
        void MapDelete(const std::string& pattern, Handler handler);
    private:
        void Map(const std::string& method, const std::string& pattern, Handler handler);
        struct RouteEntry {
            std::string method;
            RoutePattern pattern;
            Handler handler;
        };
        std::vector<RouteEntry> routes;
    };
}
