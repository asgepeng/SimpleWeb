#pragma once
#include "initiator.h"
#include "sslcm.h"
#include "sslconn.h"
#include "wthreadpool.h"
#include "reqrouter.h"
#include "logger.h"

#include <winsock2.h>
#include <windows.h>
#include <openssl/ssl.h>
#include <vector>

namespace Web
{
    class IOCPServer {
    public:
        IOCPServer();
        ~IOCPServer();
        bool Initialize(const std::string& certFile, const std::string& keyFile, int port);
        void Run();
        void Stop();
        void MapController(ControllerRoutes rounteConfig);

    private:
        void AcceptLoop();

        SOCKET listenSocket;
        HANDLE hIOCP;
        bool running = false;

        SSLContextManager sslManager;
        //ConnectionManager connManager;
        WorkerThreadPool threadPool;
        RequestRouter router;
        Logger logger;
    };
}
