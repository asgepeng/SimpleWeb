#define _CRT_SECURE_NO_WARNINGS

#include "response.h"
#include <ctime>
#include <string>
#include <cstdio>

namespace Web {

    HttpResponse::HttpResponse() :
        statusCode_(200),
        statusDescription_("OK"),
        contentType_("text/html")
    {
        // Default: Keep connection alive (HTTP/1.1 default)
        headers_["Connection"] = "keep-alive";
    }

    void HttpResponse::SetHeader(const std::string& name, const std::string& value)
    {
        headers_[name] = value;
    }
    void HttpResponse::SetCookie(const std::string& name, const std::string& value, const std::chrono::system_clock::time_point& expired)
    {
        std::time_t exp_time_t = std::chrono::system_clock::to_time_t(expired);
        char dateStr[64] = { 0 };
        std::tm tmStruct;
#if defined(_MSC_VER)
        gmtime_s(&tmStruct, &exp_time_t);
#else
        gmtime_r(&exp_time_t, &tmStruct);
#endif
        std::strftime(dateStr, sizeof(dateStr), "%a, %d %b %Y %H:%M:%S GMT", &tmStruct);

        std::string cookie;
        cookie.reserve(128);
        cookie.append(name).append("=").append(value)
            .append("; Expires=").append(dateStr)
            .append("; Path=/")
            .append("; HttpOnly");

        cookies.push_back(cookie);
    }
    void HttpResponse::SetCookie(const std::string& name, const std::string& value,
        const std::chrono::system_clock::time_point& expired,
        bool httpOnly, bool secure, const std::string& sameSite)
    {
        std::time_t exp_time = std::chrono::system_clock::to_time_t(expired);
        std::tm tmStruct;
#if defined(_MSC_VER)
        gmtime_s(&tmStruct, &exp_time);
#else
        gmtime_r(&exp_time, &tmStruct);
#endif
        char dateStr[64];
        std::strftime(dateStr, sizeof(dateStr), "%a, %d %b %Y %H:%M:%S GMT", &tmStruct);

        std::string cookie;
        cookie.reserve(128);
        cookie.append(name).append("=").append(value)
            .append("; Expires=").append(dateStr)
            .append("; Path=/");

        if (httpOnly) cookie.append("; HttpOnly");
        if (secure)   cookie.append("; Secure");
        if (!sameSite.empty()) cookie.append("; SameSite=").append(sameSite);

        cookies.push_back(std::move(cookie));
    }

    void HttpResponse::Redirect(const std::string& url)
    {
        statusCode_ = 302;
        statusDescription_ = "Found";
        SetHeader("Location", url);
    }

    void HttpResponse::Write(const std::string& content)
    {
        Body.append(content);
    }

    std::string HttpResponse::ToString() const
    {
        size_t estimatedHeaderSize = 64 + contentType_.size() + statusDescription_.size() + Body.size();
        for (const auto& header : headers_)
            estimatedHeaderSize += header.first.size() + header.second.size() + 4;

        std::string response;
        response.reserve(estimatedHeaderSize);

        // Status line
        response.append("HTTP/1.1 ")
            .append(std::to_string(statusCode_))
            .append(" ")
            .append(statusDescription_)
            .append("\r\n");

        // Content-Type
        response.append("Content-Type: ")
            .append(contentType_)
            .append("\r\n");

        // Headers
        for (const auto& header : headers_) {
            response.append(header.first)
                .append(": ")
                .append(header.second)
                .append("\r\n");
        }

        // Cookies
        for (const auto& cookie : cookies) {
            response.append("Set-Cookie: ")
                .append(cookie)
                .append("\r\n");
        }

        // Content-Length
        response.append("Content-Length: ")
            .append(std::to_string(Body.size()))
            .append("\r\n\r\n");

        // Body
        response.append(Body);
        return response;
    }
}