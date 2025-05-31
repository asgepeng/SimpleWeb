#define _CRT_SECURE_NO_WARNINGS

#include "response.h"
#include <sstream>

namespace Web
{
    HttpResponse::HttpResponse() :
        statusCode_(200),
        statusDescription_("OK"),
        contentType_("text/html"),
        Body("") { }
    void HttpResponse::SetHeader(const std::string& name, const std::string& value)
    {
        headers_[name] = value;
    }
    void HttpResponse::SetCookie(const std::string& name, const std::string& value, const std::chrono::system_clock::time_point& expired)
    {
        std::time_t exp_time_t = std::chrono::system_clock::to_time_t(expired);
        std::tm tm = *std::gmtime(&exp_time_t);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%a, %d %b %Y %H:%M:%S GMT");

        std::string cookie = name + "=" + value + "; Expires=" + oss.str() + "; Path=/";

        headers_["Set-Cookie"] = cookie;
    }
    void HttpResponse::Redirect(const std::string& url)
    {
        statusCode_ = 302;
        statusDescription_ = "Found";
        SetHeader("Location", url);
    }
    void HttpResponse::Write(const std::string& content)
    {
        Body += content;
    }
    std::string HttpResponse::ToString() const
    {
        std::ostringstream oss;
        oss << "HTTP/1.1 " << statusCode_ << " " << statusDescription_ << "\r\n";
        oss << "Content-Type: " << contentType_ << "\r\n";
        for (const auto& [key, value] : headers_) {
            oss << key << ": " << value << "\r\n";
        }
        oss << "Content-Length: " << Body.size() << "\r\n\r\n";
        oss << Body;
        return oss.str();
    }
}