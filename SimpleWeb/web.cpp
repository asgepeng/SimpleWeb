#include "web.h"
#include "mvc.h"

#include <sstream>
#include <algorithm>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")
namespace Web
{
    //STATIC FILE HANDLER
    std::string StaticFileHandler::ConvertToPathWindows(const std::string& url)
    {
        std::string localPath = "E:\\PointOfSale\\SimpleWeb\\x64\\Release\\wwwroot" + url;
        for (auto& ch : localPath)
        {
            if (ch == '/') ch = '\\';
        }
        return localPath;
    }

    std::string StaticFileHandler::GetContentType(const std::string& path)
    {
        static const std::unordered_map<std::string, std::string> mimeTypes = 
        {
            {".html", "text/html"}, {".css", "text/css"}, {".js", "application/javascript"},
            {".png", "image/png"}, {".jpg", "image/jpeg"}, {".jpeg", "image/jpeg"},
            {".gif", "image/gif"}, {".svg", "image/svg+xml"}, {".ico", "image/x-icon"},
            {".json", "application/json"}, {".txt", "text/plain"}, {".xml", "application/xml"},
            {".pdf", "application/pdf"}, {".zip", "application/zip"}, {".rar", "application/vnd.rar"},
            {".mp3", "audio/mpeg"}, {".mp4", "video/mp4"}, {".webm", "video/webm"},
            {".woff", "font/woff"}, {".woff2", "font/woff2"}, {".ttf", "font/ttf"}, {".otf", "font/otf"}
        };

        size_t dot = path.find_last_of('.');
        if (dot != std::string::npos) 
        {
            std::string ext = path.substr(dot);
            auto it = mimeTypes.find(ext);
            if (it != mimeTypes.end()) return it->second;
        }
        return "application/octet-stream";
    }

    bool StaticFileHandler::TryHandleRequest(const HttpRequest& request, SOCKET clientSocket)
    {
        if (!IsFileRequest(request.url)) return false;

        std::string fullPath = ConvertToPathWindows(request.url);
        std::ifstream file(fullPath, std::ios::binary);
        if (!file.is_open()) return false;
        file.close();

        SendFile(clientSocket, fullPath);
        return true;
    }

    void StaticFileHandler::SendFile(SOCKET clientSocket, const std::string& filePath)
    {
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::string notFound = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nFile not found.";
            send(clientSocket, notFound.c_str(), (int)notFound.size(), 0);
            return;
        }

        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::string header =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: " + GetContentType(filePath) + "\r\n"
            "Content-Length: " + std::to_string(fileSize) + "\r\n"
            "Connection: close\r\n\r\n";
        send(clientSocket, header.c_str(), (int)header.size(), 0);

        const int BUFFER_SIZE = 8192;
        char buffer[BUFFER_SIZE];
        while (file.read(buffer, BUFFER_SIZE) || file.gcount() > 0) {
            send(clientSocket, buffer, static_cast<int>(file.gcount()), 0);
        }
    }
    bool StaticFileHandler::IsFileRequest(const std::string& url)
    {
        size_t dot = url.find_last_of('.');
        size_t slash = url.find_last_of('/');
        return dot != std::string::npos && (slash == std::string::npos || dot > slash);
    }
    //END OF STATIC FILE HANDLER
    //HTTP RESPONSE
    HttpResponse::HttpResponse(SOCKET& winsock) :
        socket(winsock),
        StatusCode(200),
        StatusDescription("OK"),
        ContentType("text/html"),
        Body("") { }
    void HttpResponse::SetHeader(const std::string& name, const std::string& value)
    {
        headers_[name] = value;
    }
    void HttpResponse::Redirect(const std::string& url)
    {
        StatusCode = 302;
        StatusDescription = "Found";
        SetHeader("Location", url);
    }
    void HttpResponse::Write(const std::string& content) 
    {
        Body += content;
    }
    std::string HttpResponse::ToString() const 
    {
        std::ostringstream oss;
        oss << "HTTP/1.1 " << StatusCode << " " << StatusDescription << "\r\n";
        oss << "Content-Type: " << ContentType << "\r\n";
        for (const auto& [key, value] : headers_) {
            oss << key << ": " << value << "\r\n";
        }
        oss << "Content-Length: " << Body.size() << "\r\n\r\n";
        oss << Body;
        return oss.str();
    }

    HttpRequest::HttpRequest(const char* rawRequest, SOCKET clientSocket)
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
        Socket = clientSocket;
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

    void Router::addRoute(const std::string& method, const std::string& pattern, Handler handler) {
        routes_.push_back(RouteEntry{ method, RoutePattern(pattern), std::move(handler) });
    }

    std::string Router::handleRequest(HttpContext& ctx) 
    {
        for (auto& route : routes_) 
        {
            if (route.method == ctx.Request.method) 
            {
                std::unordered_map<std::string, std::string> routeValues;
                if (route.pattern.match(ctx.Request.url, routeValues))
                {
                    ctx.RouteData = std::move(routeValues);
                    return route.handler(ctx);
                }
            }
        }
        return "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    }

    Server::Server(unsigned short port)
        : port_(port), listenSocket_(INVALID_SOCKET), running_(false), router_(nullptr) {}

    bool Server::start()
    {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return false;

        listenSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket_ == INVALID_SOCKET) return false;

        sockaddr_in service{};
        service.sin_family = AF_INET;
        service.sin_addr.s_addr = INADDR_ANY;
        service.sin_port = htons(port_);

        if (bind(listenSocket_, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) return false;
        if (listen(listenSocket_, SOMAXCONN) == SOCKET_ERROR) return false;

        running_ = true;
        threads_.emplace_back(&Server::acceptLoop, this);
        return true;
    }
    void Server::stop()
    {
        running_ = false;
        closesocket(listenSocket_);
        WSACleanup();
    }
    void Server::setRouter(Router* router)
    {
        router_ = router;
    }
    void Server::acceptLoop()
    {
        while (running_)
        {
            SOCKET client = accept(listenSocket_, nullptr, nullptr);
            if (client == INVALID_SOCKET) continue;
            threads_.emplace_back(&Server::handleClient, this, client);
            threads_.back().detach();
        }
    }
    void Server::handleClient(SOCKET clientSocket)
    {
        char buffer[1024];
        int received = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (received > 0)
        {
            buffer[received] = '\0';

            HttpRequest request(buffer, clientSocket);
            if (!StaticFileHandler::TryHandleRequest(request, clientSocket))
            {
                HttpContext ctx(request);
                std::string response = router_ ? router_->handleRequest(ctx) : "HTTP/1.1 404 Not Found\r\n\r\n";
                send(clientSocket, response.c_str(), (int)response.size(), 0);
            }
        }
        closesocket(clientSocket);
    }
    RoutePattern::RoutePattern(const std::string& pattern)
    {
        std::istringstream iss(pattern);
        std::string segment;
        while (std::getline(iss, segment, '/')) 
        {
            if (!segment.empty())
            {
                if (segment.front() == '{' && segment.back() == '}') {
                    segments_.push_back(segment.substr(1, segment.size() - 2));
                    isParam_.push_back(true);
                }
                else {
                    segments_.push_back(segment);
                    isParam_.push_back(false);
                }
            }
        }
    }
    bool RoutePattern::match(const std::string& path, std::unordered_map<std::string, std::string>& routeValues) const
    {
        std::istringstream iss(path);
        std::string segment;
        std::vector<std::string> pathSegments;

        while (std::getline(iss, segment, '/'))
        {
            if (!segment.empty()) {
                pathSegments.push_back(segment);
            }
        }

        if (pathSegments.size() != segments_.size())
            return false;

        for (size_t i = 0; i < segments_.size(); ++i) 
        {
            if (isParam_[i]) {
                routeValues[segments_[i]] = pathSegments[i];
            }
            else if (segments_[i] != pathSegments[i]) {
                return false;
            }
        }

        return true;
    }
}