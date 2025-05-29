#pragma once

#include "request.h"
#include <openssl/ssl.h>

namespace Web
{
    class StaticFileHandler
    {
    public:
        static bool TryHandleRequest(const HttpRequest& request, SOCKET clientSocket);
        static bool TryHandleRequest(const HttpRequest& request, SSL* ssl);
    private:
        static std::string ConvertToPathWindows(const std::string& url);
        static std::string GetContentType(const std::string& path);
        static bool IsFileRequest(const std::string& url);
        static void SendFile(SOCKET clientSocket, const std::string& filePath);
        static void SendFile(SSL* ssl, const std::string& filePath);
    };
}

