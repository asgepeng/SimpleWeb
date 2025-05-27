#pragma once
#include "web.h"
#include "db.h"
#include "staticfile.h"
#include "threadpool.h"

#include <atomic>

#pragma comment(lib, "ws2_32.lib")
namespace Web
{
    Server::Server() : listener(INVALID_SOCKET), isrun(false), routerPtr(nullptr)
    {
        routerPtr = std::make_unique<Router>();
    }

    bool Server::Run(const unsigned short port )
    {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return false;

        listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listener == INVALID_SOCKET) return false;

        sockaddr_in service{};
        service.sin_family = AF_INET;
        service.sin_addr.s_addr = INADDR_ANY;
        service.sin_port = htons(port);

        if (bind(listener, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) return false;
        if (listen(listener, SOMAXCONN) == SOCKET_ERROR) return false;

        isrun = true;
        threads.emplace_back(&Server::AcceptLoop, this);
        return true;
    }
    void Server::Stop()
    {
        isrun = false;
        closesocket(listener);
        WSACleanup();
    }
    void Server::AcceptLoop()
    {
        ThreadPool pool(std::thread::hardware_concurrency());

        while (isrun)
        {
            SOCKET client = accept(listener, nullptr, nullptr);
            if (client == INVALID_SOCKET) continue;

            pool.enqueue([this, client] {
                HandleRequest(client);
                });
        }
    }
    void Server::HandleRequest(SOCKET clientSocket)
    {
        std::string data;
        char buffer[1024];
        int received;

        // read until no more data or error
        while ((received = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0)
        {
            data.append(buffer, received);
            if (data.find("\r\n\r\n") != std::string::npos)
                break; // end of HTTP headers
        }
        if (received <= 0)
        {
            closesocket(clientSocket);
            return;
        }

        HttpRequest request(data.c_str(), clientSocket);
        if (!StaticFileHandler::TryHandleRequest(request, clientSocket))
        {
            HttpContext ctx(request);
            routerPtr->HandleRequest(ctx);
        }
        closesocket(clientSocket);
    }
}