#include "httpcontext.h"

std::string Web::HttpContext::getParam(const std::string& key) const
{
    auto it = routeData.find(key);
    return it != routeData.end() ? it->second : "";
}
