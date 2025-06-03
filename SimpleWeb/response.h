#pragma once
#include "request.h"
#include <chrono>

namespace Web
{
    class HttpResponse {
    public:
        HttpResponse();

        void SetHeader(const std::string& name, const std::string& value);
        void SetCookie(const std::string& name, const std::string& value, const std::chrono::system_clock::time_point& expired);
        void Redirect(const std::string& url);
        void Write(const std::string& content);
        std::string ToString() const;

        void StatusCode(const int statusCode) { statusCode_ = statusCode; }
        int StatusCode() { return statusCode_; }

        void StatusDescription(const std::string& sdescription) { statusDescription_ = sdescription; }
        std::string StatusDescription() { return statusDescription_; }

        void ContentType(const std::string& contentType) { contentType_ = contentType; }
        std::string ContentType() { return contentType_; }
    private:
        std::unordered_map<std::string, std::string> headers_;
        std::vector<std::string> cookies;
        int statusCode_;
        std::string statusDescription_;
        std::string contentType_;
        std::string Body;
    };
}

