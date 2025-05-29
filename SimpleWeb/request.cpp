#include "request.h"
#include <sstream>

namespace Web
{
    static std::string decodeUrl(const std::string& s) {
        std::string result;
        for (size_t i = 0; i < s.length(); ++i) {
            if (s[i] == '+') {
                result += ' ';
            }
            else if (s[i] == '%' && i + 2 < s.length()) {
                std::string hex = s.substr(i + 1, 2);
                char ch = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
                result += ch;
                i += 2;
            }
            else {
                result += s[i];
            }
        }
        return result;
    }

    HttpRequest::HttpRequest(const char* rawRequest)
    {
        std::istringstream stream(rawRequest);
        std::string line;
        if (std::getline(stream, line))
        {
            std::istringstream reqLine(line);
            reqLine >> method;
            std::string fullPath;
            reqLine >> fullPath;
            reqLine >> httpVersion;

            size_t qPos = fullPath.find('?');
            if (qPos != std::string::npos)
            {
                url = fullPath.substr(0, qPos);
                parseQueryString(fullPath.substr(qPos + 1));
            }
            else {
                url = fullPath;
            }
        }
        while (std::getline(stream, line))
        {
            if (line == "\r" || line.empty())
                break;

            size_t colon = line.find(':');
            if (colon != std::string::npos)
            {
                std::string key = line.substr(0, colon);
                std::string value = line.substr(colon + 1);
                trim(key);
                trim(value);
                headers[key] = value;

                if (key == "Cookie")
                {
                    parseCookies(value);
                }
            }
        }

        std::ostringstream bodyStream;
        while (std::getline(stream, line))
        {
            bodyStream << line << "\n";
        }
        body = bodyStream.str();
    }
    std::string HttpRequest::getHeader(const std::string& name) const
    {
        auto it = headers.find(name);
        return (it != headers.end()) ? it->second : "";
    }
    std::string HttpRequest::getCookie(const std::string& name) const
    {
        auto it = cookies.find(name);
        return (it != cookies.end()) ? it->second : "";
    }
    std::string HttpRequest::getQueryParam(const std::string& name) const
    {
        auto it = queryParams.find(name);
        return (it != queryParams.end()) ? it->second : "";
    }
    FormCollection HttpRequest::getFormCollection()
    {
        FormCollection form;

        std::istringstream stream(body);
        std::string pair;

        while (std::getline(stream, pair, '&')) {
            auto pos = pair.find('=');
            if (pos != std::string::npos) {
                std::string key = decodeUrl(pair.substr(0, pos));
                std::string value = decodeUrl(pair.substr(pos + 1));

                Web::HttpRequest::trim(key);
                Web::HttpRequest::trim(value);

                form[key] = value;
            }
        }

        return form;
    }
    void HttpRequest::trim(std::string& s)
    {
        s.erase(0, s.find_first_not_of(" \t\r\n"));
        s.erase(s.find_last_not_of(" \t\r\n") + 1);
    }
    void HttpRequest::parseQueryString(const std::string& query)
    {
        std::istringstream qs(query);
        std::string pair;
        while (std::getline(qs, pair, '&'))
        {
            size_t eq = pair.find('=');
            if (eq != std::string::npos)
            {
                std::string key = pair.substr(0, eq);
                std::string val = pair.substr(eq + 1);
                queryParams[key] = val;
            }
        }
    }
    void HttpRequest::parseCookies(const std::string& cookieHeader)
    {
        std::istringstream ss(cookieHeader);
        std::string pair;
        while (std::getline(ss, pair, ';'))
        {
            size_t eq = pair.find('=');
            if (eq != std::string::npos)
            {
                std::string key = pair.substr(0, eq);
                std::string val = pair.substr(eq + 1);
                trim(key);
                trim(val);
                cookies[key] = val;
            }
        }
    }
}