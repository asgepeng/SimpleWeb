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
    }

    void HttpResponse::SetHeader(const std::string& name, const std::string& value)
    {
        headers_.insert_or_assign(name, value);
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

    void HttpResponse::Redirect(const std::string& url)
    {
        statusCode_ = 302;
        statusDescription_ = "Found";
        SetHeader("Location", url);
    }

    void HttpResponse::Write(const std::string& content)
    {
        Body.reserve(Body.size() + content.size()); // Avoid multiple reallocations
        Body.append(content);
    }

    std::string HttpResponse::ToString() const
    {
        size_t estimatedHeaderSize = 64 + contentType_.size() + statusDescription_.size();
        for (const auto& header : headers_) {
            estimatedHeaderSize += header.first.size() + header.second.size() + 4;
        }

        std::string response;
        response.reserve(estimatedHeaderSize + Body.size());

        response.append("HTTP/1.1 ")
            .append(std::to_string(statusCode_))
            .append(" ")
            .append(statusDescription_)
            .append("\r\n");

        response.append("Content-Type: ")
            .append(contentType_)
            .append("\r\n");

        for (const auto& header : headers_) {
            response.append(header.first)
                .append(": ")
                .append(header.second)
                .append("\r\n");
        }
        for (const auto& cookie : cookies)
        {
            response.append("Set-Cookie: ")
                .append(cookie);
        }
        response.append("Content-Length: ")
            .append(std::to_string(Body.size()))
            .append("\r\n\r\n");

        response.append(Body);
        return response;
    }

}
