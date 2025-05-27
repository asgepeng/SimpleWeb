#pragma once
#include "routing.h"
#include <thread>

namespace Web
{
    class Server
    {
    public:
        Server();
        bool Run(const unsigned short port);
        void Stop();
        void UseController(Web::RouteConfig& initializer) 
        {
            initializer.RegisterEndPoints(routerPtr.get());
        }
    private:
        void AcceptLoop();
        void HandleRequest(SOCKET clientSocket);

        SOCKET listener;

        std::atomic<bool> isrun;
        std::unique_ptr<Router> routerPtr;
        std::vector<std::thread> threads;
    };
}