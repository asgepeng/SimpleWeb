#pragma once
#include <string>
#include <vector>
#include <openssl/ssl.h>

namespace Web::Mvc
{
	class ActionResult
	{
	public:
        virtual std::string ToString() = 0;
	};

    class ViewResult : public ActionResult
    {
    public:
        std::string ToString() override
        {
            return std::string();
        }
        std::string layout = "";
        std::string sectionBody = "";
        std::string sectionStyles = "";
        std::string sectionScripts = "";
    };

    class ContentResult : public ActionResult
    {
    public:
        std::string content;
        std::string contentType = "text/plain";

        ContentResult(const std::string& c, const std::string& ct = "text/plain")
            : content(c), contentType(ct) {}

        std::string ToString() override 
        {
            return "HTTP/1.1 200 OK\r\nContent-Type: " + contentType + "\r\n\r\n" + content;
        }
    };

    class JsonResult : public ActionResult {
    public:
        std::string json;

        JsonResult(const std::string& j) : json(j) {}

        std::string ToString() override {
            return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + json;
        }
    };

    class FileResult : public ActionResult 
    {
    public:
        std::string contentType;
        FileResult(const std::string& ct) : contentType(ct) {}
    };

    class FileContentResult : public FileResult
    {
    public:
        std::vector<char> data;

        FileContentResult(const std::vector<char>& d, const std::string& ct)
            : FileResult(ct), data(d) {}

        std::string ToString() override {
            return "HTTP/1.1 200 OK\r\nContent-Type: " + contentType +
                "\r\nContent-Length: " + std::to_string(data.size()) + "\r\n\r\n" +
                std::string(data.begin(), data.end());
        }
    };

    class RedirectResult : public ActionResult
    {
    public:
        std::string url;

        RedirectResult(const std::string& u) : url(u) {}

        std::string ToString() override {
            return "HTTP/1.1 302 Found\r\nLocation: " + url + "\r\n\r\n";
        }
    };

    class StatusCodeResult : public ActionResult 
    {
    public:
        int statusCode;

        StatusCodeResult(int code) : statusCode(code) {}

        std::string ToString() override {
            return "HTTP/1.1 " + std::to_string(statusCode) + " \r\n\r\n";
        }
    };

    class OkResult : public StatusCodeResult 
    {
    public:
        OkResult() : StatusCodeResult(200) {}
    };

    class OkObjectResult : public ActionResult
    {
    public:
        std::string content;

        OkObjectResult(const std::string& c) : content(c) {}

        std::string ToString() override 
        {
            return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + content;
        }
    };

    class BadRequestResult : public StatusCodeResult
    {
    public:
        BadRequestResult() : StatusCodeResult(400) {}
    };

    class BadRequestObjectResult : public ActionResult
    {
    public:
        std::string message;

        BadRequestObjectResult(const std::string& msg) : message(msg) {}

        std::string ToString() override 
        {
            return "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\n" + message;
        }
    };

    class NotFoundResult : public StatusCodeResult 
    {
    public:
        NotFoundResult() : StatusCodeResult(404) {}
    };

    class NotFoundObjectResult : public ActionResult 
    {
    public:
        std::string message;
        NotFoundObjectResult(const std::string& msg) : message(msg) {}

        std::string ToString() override {
            return "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n" + message;
        }
    };

    class NoContentResult : public StatusCodeResult 
    {
    public:
        NoContentResult() : StatusCodeResult(204) {}
    };

    class UnauthorizedResult : public StatusCodeResult 
    {
    public:
        UnauthorizedResult() : StatusCodeResult(401) {}
    };

    class ForbidResult : public StatusCodeResult 
    {
    public:
        ForbidResult() : StatusCodeResult(403) {}
    };

    class EmptyResult : public ActionResult 
    {
    public:
        std::string ToString() override {
            return ""; 
        }
    };

}

