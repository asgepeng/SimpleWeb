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