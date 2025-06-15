#include "routing.h"
#include "requesthandler.h"
#include "sslcm.h"

#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <future>

#pragma comment(lib, "ws2_32.lib")

using namespace std::chrono;
namespace Web
{
    enum class Operation { Accept, Receive, Send, SendReady };

    struct IOContext
    {
        OVERLAPPED overlapped = {};
        WSABUF wsabuf;
        SOCKET socket = INVALID_SOCKET;
        SSL* ssl = nullptr;
        BIO* rbio = nullptr;
        BIO* wbio = nullptr;
        Operation operationType;
        char buffer[8192]{};
        steady_clock::time_point lastActive;

        std::string receiveBuffer = "";
        std::string sendBuffer = "";

        size_t lastSentPlaintext = 0;
        size_t dataTransfered = 0;

        Web::HttpRequest* request = nullptr;
        bool HandshakeDone();
        bool ReceiveComplete();
        bool PrepareSSL(SSL_CTX* sslContext);

        IOContext();
        ~IOContext();
    };

    // Main IOCP server class
    class Server
    {
    public:
        Server();
        ~Server();
        bool Start();
        void Stop();
        void MapControllers(Web::RouteConfig* config);

        void UseSSL(bool value) { if (!running) useSSL = value; }
        bool UseSSL() { return useSSL; }
        void Port(USHORT value) { if (!running) useSSL = value; }
        USHORT Port() { return port; }
        bool routeInitialized = false;
    private:
        bool useSSL = false;
        USHORT port = 0;

        HANDLE hIOCP;

        SOCKET listenerSocket;
        SslContextManager sslManager;
        std::vector<std::thread> workerThreads;
        std::atomic<bool> running;
        std::mutex clientsMutex;
        std::vector<SOCKET> clients;
        RequestHandler handler;

        LPFN_ACCEPTEX lpfnAcceptEx = nullptr;
        LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockaddrs = nullptr;

        void Shutdown();
        void WorkerThread();
        void SslWorkerThread();
        void LogSslError(Operation operationType, int err);

        bool PostAccept();
        bool PostReceive(IOContext* ctx);
        bool PostSend(IOContext* ctx);

        void CleanupContext(IOContext* ctx);
        void HandleRequest(IOContext* ctx);
    };
}