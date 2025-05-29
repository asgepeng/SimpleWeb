#pragma once
#include "response.h"
#include "identity.h"

#include <openssl/ssl.h>

namespace Web
{
    class HttpContext {
    public:
        HttpContext(const HttpRequest& req, SSL* sslPtr) : request(req), socket(INVALID_SOCKET), ssl(sslPtr) { }
        HttpContext(const HttpRequest& req, SOCKET clientSocket) : request(req), socket(clientSocket), ssl(nullptr) { }
        HttpRequest request;
        HttpResponse response;
        std::string getParam(const std::string& key) const;
        std::unordered_map<std::string, std::string> routeData;

        Authentication::User User;
        const SOCKET socket;
        SSL* ssl;
    };
}
