#include "sslconn.h"
#include <mswsock.h>
#include <openssl/err.h>
#include <iostream>

Web::Connection::Connection(SOCKET sock, SSL_CTX* ctx) : socket(sock), ssl(nullptr) {
    ssl = SSL_new(ctx);
    if (ssl) {
        BIO* rbio = BIO_new(BIO_s_mem());
        BIO* wbio = BIO_new(BIO_s_mem());
        SSL_set_bio(ssl, rbio, wbio);
        SSL_set_accept_state(ssl);
    }
}

Web::Connection::~Connection()
{
    Cleanup();
}

bool Web::Connection::StartHandshake(HANDLE hIOCP)
{
    CreateIoCompletionPort((HANDLE)socket, hIOCP, (ULONG_PTR)this, 0);

    PER_IO_DATA* pIo = new PER_IO_DATA();
    ZeroMemory(pIo, sizeof(PER_IO_DATA));
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

void Web::Connection::Cleanup()
{
    if (ssl) 
    {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl = nullptr;
    }
    if (socket != INVALID_SOCKET) {
        closesocket(socket);
        socket = INVALID_SOCKET;
    }
}

bool Web::Connection::PostReceive()
{
    auto* io = new PER_IO_DATA();
    ZeroMemory(&io->overlapped, sizeof(OVERLAPPED));
    io->wsaBuf.buf = io->buffer;
    io->wsaBuf.len = sizeof(io->buffer);
    io->operation = OP_READ;
    DWORD flags = 0;
    int res = WSARecv(socket, &io->wsaBuf, 1, nullptr, &flags, &io->overlapped, nullptr);
    if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        delete io;
        return false;
    }
    return true;
}

void Web::ConnectionManager::Add(Connection* conn)
{
    std::lock_guard<std::mutex> lock(mutex);
    connections.insert(conn);
}

void Web::ConnectionManager::Remove(Connection* conn)
{
    std::lock_guard<std::mutex> lock(mutex);
    connections.erase(conn);
}

void Web::ConnectionManager::CleanupAll()
{
    std::lock_guard<std::mutex> lock(mutex);
    for (auto conn : connections) {
        conn->Cleanup();
        delete conn;
    }
    connections.clear();
}
