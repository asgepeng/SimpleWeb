#pragma once
#include "routing.h"
#include <thread>
#include <openssl/ssl.h>
#include <openssl/err.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")

namespace Web
{
    class Server
    {
    public:
        Server();
        bool Run(const unsigned short port = 443);
        void Stop();
        void MapController(Web::RouteConfig& initializer) 
        {
            initializer.RegisterEndPoints(routerPtr.get());
        }
    private:
        void InitSSL();
        void AcceptLoop();
        void HandleRequest(SOCKET clientSocket);

        SOCKET listener;

        std::atomic<bool> isrun;
        std::unique_ptr<Router> routerPtr;
        std::vector<std::thread> threads;

        SSL_CTX* sslCtx = nullptr;
    };
}