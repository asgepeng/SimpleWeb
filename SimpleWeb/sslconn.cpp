#include "staticfile.h"
#include "sslconn.h"
#include "routing.h"

#include <mswsock.h>
#include <openssl/err.h>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

namespace Web
{
    Connection::Connection(SOCKET sock, SSL_CTX* ctx) : socket(sock), ssl(nullptr) {
        ssl = SSL_new(ctx);
        if (ssl) {
            BIO* rbio = BIO_new(BIO_s_mem());
            BIO* wbio = BIO_new(BIO_s_mem());
            SSL_set_bio(ssl, rbio, wbio);
            SSL_set_accept_state(ssl);
        }
    }

    Connection::~Connection()
    {
        Cleanup();
    }

    bool Connection::StartHandshake(HANDLE hIOCP)
    {
        CreateIoCompletionPort((HANDLE)socket, hIOCP, (ULONG_PTR)this, 0);

        IOData* pIo = new IOData();
        ZeroMemory(pIo, sizeof(IOData));
        pIo->wsaBuf.buf = pIo->buffer;
        pIo->wsaBuf.len = sizeof(pIo->buffer);
        pIo->operation = OP_HANDSHAKE;

        DWORD flags = 0;
        int ret = WSARecv(socket, &pIo->wsaBuf, 1, NULL, &flags, &pIo->overlapped, NULL);
        if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            std::cerr << "WSARecv failed on initial handshake." << std::endl;
            delete pIo;
            return false;
        }

        return true;
    }
    bool Connection::HandshakeFinished()
    {
        return SSL_is_init_finished(ssl);
    }
    bool Connection::Accept(int& sslError)
    {
        int ret = SSL_accept(ssl);
        if (ret > 0)
            return true;

        sslError = SSL_get_error(ssl, ret);
        if (sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE)
        {
            return false;
        }

        ERR_print_errors_fp(stderr);
        return false;
    }
    void Connection::Cleanup()
    {
        if (ssl)
        {
            SSL_shutdown(ssl);
            SSL_free(ssl);
            ssl = nullptr;
        }
        if (socket != INVALID_SOCKET) 
        {
            closesocket(socket);
            socket = INVALID_SOCKET;
        }
    }

    std::string Connection::GetIPAddress()
    {
        sockaddr_in addr;
        int addrLen = sizeof(addr);
        if (getpeername(socket, (sockaddr*)&addr, &addrLen) == 0)
        {
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(addr.sin_addr), ipStr, INET_ADDRSTRLEN);
            return std::string(ipStr);
        }
        return "";

    }

    void ConnectionManager::Add(Connection* conn)
    {
        std::lock_guard<std::mutex> lock(mutex);
        connections.insert(conn);
    }

    void ConnectionManager::Remove(Connection* conn)
    {
        std::lock_guard<std::mutex> lock(mutex);
        connections.erase(conn);
    }

    void ConnectionManager::CleanupAll()
    {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto conn : connections) 
        {
            conn->Cleanup();
            delete conn;
        }
        connections.clear();
    }
}
