#include "routing.h"

#include <winsock2.h>
#include <mswsock.h>
#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <openssl/ssl.h>

#pragma comment(lib, "ws2_32.lib")

namespace Web
{
    constexpr size_t MAX_REQUEST_SIZE = 16 * 1024;
    // Operation enum
    enum class Operation { Accept, Handshake, Receive, Send };

    // Context struct for each I/O operation
    struct IOContext
    {
        OVERLAPPED overlapped = {};
        WSABUF wsabuf;
        SOCKET socket = INVALID_SOCKET;
        SSL* ssl = nullptr;
        Operation operationType;

        char buffer[4096]{};
        std::chrono::steady_clock::time_point lastActive;

        std::string data = "";
        size_t lastSentPlaintext = 0;
        size_t dataTransfered = 0;

        Web::HttpRequest* request = nullptr;
        IOContext() : operationType(Operation::Accept)
        {
            wsabuf.buf = buffer;
            wsabuf.len = sizeof(buffer);
            lastActive = std::chrono::steady_clock::now();
        }
        ~IOContext()
        {
            if (request != nullptr)
            {
                delete request;
                request = nullptr;
            }
            if (ssl != nullptr)
            {
                SSL_free(ssl);
            }
        }
        bool handshakeDone = false;
    };

    // Main IOCP server class
    class Server
    {
    public:
        Server() : hIOCP(NULL), listenerSocket(INVALID_SOCKET), running(false), sslContext(nullptr), useSSL(false) {}
        bool Start();
        void Stop();
        void MapControllers(Web::RouteConfig* config);

        void UseSSL(bool value)
        {
            if (!running)
            {
                useSSL = value;
            }
        }
        bool UseSSL() { return useSSL; }
        void Port(USHORT value)
        {
            if (!running)
            {
                port = value;
            }
        }
        USHORT Port() { return port; }
    private:
        bool useSSL = false;
        USHORT port = 0;

        HANDLE hIOCP;

        SOCKET listenerSocket;
        SSL_CTX* sslContext;
        std::vector<std::thread> workerThreads;
        std::atomic<bool> running;
        std::mutex clientsMutex;
        std::vector<SOCKET> clients;

        LPFN_ACCEPTEX lpfnAcceptEx = nullptr;
        LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockaddrs = nullptr;

        bool InitializeSsl();
        void Shutdown();
        void WorkerThread();
        void WorkerThreadSsl();
        bool PostAccept();
        void PostReceive(IOContext* ctx);
        void PrepareHandshake(IOContext* ctx, int sslError);
        void SendPendingBIO(IOContext* ctx);
        void CleanupSocket(SOCKET s);
        void CleanupContext(IOContext* ctx);
        void DisconnectClient(SOCKET s);
        void HandleRequest(Web::HttpRequest& request, std::string& response);
    };
}