#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <atomic>
#include <thread>
#include <vector>
#include <functional>
#include <map>
#include <unordered_map>
#include <iostream>

namespace Web
{
    class HttpRequest {
    public:
        HttpRequest(const char* rawRequest, SOCKET clientSocket);
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

        SOCKET Socket;
    private:
        static void trim(std::string& s);
        void parseQueryString(const std::string& query);
        void parseCookies(const std::string& cookieHeader);
    };

    class StaticFileHandler
    {
    public:
        static bool TryHandleRequest(const HttpRequest& request, SOCKET clientSocket);
    private:
        static std::string ConvertToPathWindows(const std::string& url);
        static std::string GetContentType(const std::string& path);
        static bool IsFileRequest(const std::string& url);
        static void SendFile(SOCKET clientSocket, const std::string& filePath);
    };

    class HttpResponse {
    public:
        int StatusCode;
        std::string StatusDescription;
        std::string ContentType;
        std::string Body;

        HttpResponse(SOCKET& clientSocket);

        void SetHeader(const std::string& name, const std::string& value);
        void Redirect(const std::string& url);
        void Write(const std::string& content);
        std::string ToString() const;

    private:
        SOCKET socket;
        std::unordered_map<std::string, std::string> headers_;
    };

    class HttpContext {
    public:
        HttpRequest Request;
        HttpResponse Response;
        
        std::unordered_map<std::string, std::string> RouteData;

        HttpContext(const HttpRequest& req)
            : Request(req), Response(Request.Socket) {}
    };

    class RoutePattern
    {
    public:
        explicit RoutePattern(const std::string& pattern);
        bool match(const std::string& path, std::unordered_map<std::string, std::string>& routeValues) const;

    private:
        std::vector<std::string> segments_;
        std::vector<bool> isParam_;
    };

    class Router
    {
    public:
        using Handler = std::function<std::string(HttpContext&)>;

        void addRoute(const std::string& method, const std::string& pattern, Handler handler);
        std::string handleRequest(HttpContext& ctx);

    private:
        struct RouteEntry {
            std::string method;
            RoutePattern pattern;
            Handler handler;
        };

        std::vector<RouteEntry> routes_;
    };

    class Server
    {
    public:
        Server(unsigned short port);
        bool start();
        void stop();

        void setRouter(Router* router);
    private:
        void acceptLoop();
        void handleClient(SOCKET clientSocket);

        unsigned short port_;
        SOCKET listenSocket_;
        std::atomic<bool> running_;
        Router* router_;
        std::vector<std::thread> threads_;
    };
}