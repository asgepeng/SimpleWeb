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
        BIO* rbio = nullptr;
        BIO* wbio = nullptr;
        Operation operationType;
        char buffer[8192]{};

        std::chrono::steady_clock::time_point lastActive;

        std::string data = "";
        size_t lastSentPlaintext = 0;
        size_t dataTransfered = 0;
        Web::HttpRequest* request = nullptr;
        bool HandshakeDone()
        {
            return SSL_is_init_finished(ssl) > 0;
        }
        IOContext() : operationType(Operation::Accept)
        {
            wsabuf.buf = buffer;
            wsabuf.len = sizeof(buffer);
            lastActive = std::chrono::steady_clock::now();
        }

        ~IOContext()
        {
            CleanupSSL();
            if (request != nullptr)
            {
                delete request;
                request = nullptr;
            }
        }

        void CloseSocket()
        {
            if (socket != INVALID_SOCKET) {
                closesocket(socket);
                socket = INVALID_SOCKET;
            }
        }
        void CleanupSSL()
        {
            if (ssl != nullptr)
            {
                SSL_free(ssl);
                ssl = nullptr;
            }   
        }
        void CleanupBIO()
        {
            if (rbio) BIO_free(rbio);
            if (wbio) BIO_free(wbio);
        }
    };

    // Main IOCP server class
    class Server
    {
    public:
        Server() : hIOCP(NULL), listenerSocket(INVALID_SOCKET), running(false), sslContext(nullptr), useSSL(false) {}
        ~Server()
        {
            Stop();
        }
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
        void LogSslError(Operation operationType, int err);
        bool PostAccept();
        void PostReceive(IOContext* ctx);
        bool PrepareHandshake(IOContext* ctx);
        void SendPendingBIO(IOContext* ctx);
        void CleanupSocket(SOCKET s);
        void CleanupSslContext();
        void DisconnectClient(SOCKET s);
        void HandleRequest(Web::HttpRequest& request, std::string& response);
    };
}