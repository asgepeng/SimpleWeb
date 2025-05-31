#include "routing.h"
#include "controllers.h"
#include <iostream>

namespace Web
{
    /* STATIC */
    const std::string& GetNotFoundContent()
    {
        static const std::string content = R"(<html lang="en"><head><meta charset="UTF-8"/><meta name="viewport" content="width=device-width, initial-scale=1"/><title>404 Not Found</title><style>body,html{height:100%;margin:0;font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;background:#f0f2f5;display:flex;justify-content:center;align-items:center;color:#333}.container{text-align:center;max-width:400px;padding:20px}.error-code{font-size:10rem;font-weight:900;color:#ff6b6b;margin:0}h1{font-size:2rem;margin:10px 0 20px}p{color:#666;font-size:1.1rem;margin-bottom:30px}a.button{text-decoration:none;background-color:#4a90e2;color:#fff;padding:12px 30px;border-radius:30px;font-weight:600;transition:background-color .3s ease}a.button:hover{background-color:#357abd}</style></head><body><div class="container" role="main" aria-labelledby="title" aria-describedby="desc"><h1 id="title" class="error-code">404</h1><h2 id="desc">Oops! Page not found.</h2><p>The page you are looking for might have been removed, had its name changed, or is temporarily unavailable.</p><a href="/" class="button" aria-label="Go back to homepage">Go to Homepage</a></div></body></html>)";
        return content;
    }
    /* Class RoutePattern*/
    RoutePattern::RoutePattern(const std::string& pattern)
    {
        std::istringstream iss(pattern);
        std::string segment;
        while (std::getline(iss, segment, '/'))
        {
            if (!segment.empty())
            {
                if (segment.front() == '{' && segment.back() == '}') {
                    segments_.push_back(segment.substr(1, segment.size() - 2));
                    isParam_.push_back(true);
                }
                else {
                    segments_.push_back(segment);
                    isParam_.push_back(false);
                }
            }
        }
    }
    bool RoutePattern::match(const std::string& path, std::unordered_map<std::string, std::string>& routeValues) const
    {
        std::istringstream iss(path);
        std::string segment;
        std::vector<std::string> pathSegments;

        while (std::getline(iss, segment, '/'))
        {
            if (!segment.empty()) 
            {
                pathSegments.push_back(segment);
            }
        }
        if (pathSegments.size() != segments_.size()) return false;

        for (size_t i = 0; i < segments_.size(); ++i)
        {
            if (isParam_[i]) 
            {
                routeValues[segments_[i]] = pathSegments[i];
            }
            else if (segments_[i] != pathSegments[i])
            {
                return false;
            }
        }

        return true;
    }

    /* Class Router */
    /* PRIVATE */
    HttpResponse Router::Handle(Web::HttpRequest& request)
    {
        for (auto& route : routes)
        {
            std::cout << route.method << " vs " << request.method << std::endl;
            std::cout << request.url << std::endl;
            if (route.method == request.method)
            {
                std::unordered_map<std::string, std::string> routeValues;
                if (route.pattern.match(request.url, routeValues))
                {
                    HttpContext context(request);
                    context.routeData = std::move(routeValues);
                    return route.handler(context);
                }
            }
        }
        HttpResponse response;
        response.StatusCode(404);
        response.Write(GetNotFoundContent());
        return response;
    }
    void Router::Map(const std::string& method, const std::string& pattern, Handler handler)
    {
        routes.push_back(RouteEntry{ method, RoutePattern(pattern), std::move(handler) });
    }
    /* PUBLIC */
    void Router::MapGet(const std::string& pattern, Handler handler)
    {
        Map("GET", pattern, handler);
    }
    void Router::MapPost(const std::string& pattern, Handler handler)
    {
        Map("POST", pattern, handler);
    }
    void Router::MapPut(const std::string& pattern, Handler handler)
    {
        Map("PUT", pattern, handler);
    }
    void Router::MapDelete(const std::string& pattern, Handler handler)
    {
        Map("DELETE", pattern, handler);
    }
}