#include "staticfile.h"
#include <iostream>
#include <fstream>
#include <filesystem>

namespace Web
{
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
}