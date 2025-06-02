#pragma once
#include <WinSock2.h>
#include <openssl/ssl.h>
#include <mutex>
#include <unordered_set>

namespace Web
{
    class Connection {
    public:
        Connection(SOCKET sock, SSL_CTX* ctx);
        ~Connection();

        bool StartHandshake(HANDLE hIOCP);
        void Cleanup();
        bool PostReceive();

        SOCKET GetSocket() const { return socket; }
        SSL* GetSSL() const { return ssl; }

        struct PER_IO_DATA {
            OVERLAPPED overlapped;
            WSABUF wsaBuf;
            char buffer[4096];
            DWORD operation;
        };

        enum { OP_READ = 0, OP_WRITE = 1, OP_HANDSHAKE = 2 };
    private:
        SOCKET socket;
        SSL* ssl;
    };

    class ConnectionManager {
    public:
        void Add(Connection* conn);
        void Remove(Connection* conn);
        void CleanupAll();
    private:
        std::mutex mutex;
        std::unordered_set<Connection*> connections;
    };

}

