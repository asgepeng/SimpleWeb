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
    class FileHandler
    {
    public:
        static bool IsFileRequest(std::string url);
        static std::string ConvertToPathWindows(const std::string& url);
        static std::string GetContentType(const std::string& path);
        static void SendFile(SOCKET clientSocket, const std::string& urlPath);
    };

    class HttpRequest {
    public:
        HttpRequest(const char* rawRequest);
        HttpRequest(SOCKET& socket);
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

    class HttpResponse {
    public:
        int StatusCode;
        std::string StatusDescription;
        std::string ContentType;
        std::string Body;

        HttpResponse(SOCKET& winsock);

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
        SOCKET Socket;
        HttpRequest Request;
        HttpResponse Response;

        HttpContext(SOCKET s, const HttpRequest& req)
            : Socket(s), Request(req), Response(s) {}
    };


    class Router
    {
    public:
        using Handler = std::function<std::string()>;

        void addRoute(const std::string& path, Handler handler);
        std::string handleRequest(const std::string& requestLine);

    private:
        std::unordered_map<std::string, Handler> routes_;
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