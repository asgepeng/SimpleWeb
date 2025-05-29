#pragma once
#include <winsock2.h>
#include <string>
#include <unordered_map>
#include <map>

namespace Web
{
	class FormCollection : public std::map<std::string, std::string> { };
    class HttpRequest {
    public:
        HttpRequest(const char* rawRequest);
        std::string method;
        std::string url;
        std::string httpVersion;
        std::unordered_map<std::string, std::string> headers;
        std::unordered_map<std::string, std::string> cookies;
        std::unordered_map<std::string, std::string> queryParams;
        std::string body;

        std::string getHeader(const std::string& name) const;
        std::string getCookie(const std::string& name) const;
        std::string getQueryParam(const std::string& name) const;
		FormCollection getFormCollection();
    private:
		static void trim(std::string& s);
        void parseQueryString(const std::string& query);
        void parseCookies(const std::string& cookieHeader);
    };
}
