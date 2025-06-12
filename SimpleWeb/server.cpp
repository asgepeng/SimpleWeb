#include "routing.h"
#include "server.h"
#include "staticfile.h"
#include "appconfig.h"

#include <openssl/err.h>
#include <exception>
#include <memory>

#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")

namespace Web
{
    bool Server::InitializeSsl()
    {
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();

        std::string certPath = Configuration::Get("ssl_certificate");
        std::string keyPath = Configuration::Get("ssl_key");

        sslContext = SSL_CTX_new(TLS_server_method());
        if (!sslContext)
        {
            ERR_print_errors_fp(stderr);
            CleanupSslContext();
            return false;
        }

        if (SSL_CTX_use_certificate_file(sslContext, certPath.c_str(), SSL_FILETYPE_PEM) <= 0)
        {
            ERR_print_errors_fp(stderr);
            CleanupSslContext();
            return false;
        }

        if (SSL_CTX_use_PrivateKey_file(sslContext, keyPath.c_str(), SSL_FILETYPE_PEM) <= 0)
        {
            ERR_print_errors_fp(stderr);
            CleanupSslContext();
            return false;
        }

        if (!SSL_CTX_check_private_key(sslContext))
        {
            std::cerr << "Private key does not match the public certificate\n";
            CleanupSslContext();
            return false;
        }

        std::cout << "ssl successfully initialized" << std::endl;
        return true;
    }
    bool Server::Start()
    {
        if (useSSL)
        {
            port = 443;
            if (!InitializeSsl())
            {
                CleanupSslContext();
                return false;
            }
        }
        else
        {
            if (port == 0) port = 80;
        }

        WSADATA wsaData;
        int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (res != 0)
        {
            CleanupSslContext();
            return false;
        }

        listenerSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (listenerSocket == INVALID_SOCKET)
        {
            CleanupSslContext();
            WSACleanup();
            return false;
        }

        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY; // listen all IP
        addr.sin_port = htons(port);

        if (bind(listenerSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
        {
            closesocket(listenerSocket);
            CleanupSslContext();
            WSACleanup();
            return false;
        }

        if (listen(listenerSocket, SOMAXCONN) == SOCKET_ERROR)
        {
            closesocket(listenerSocket);
            CleanupSslContext();
            WSACleanup();
            return false;
        }

        // Create IOCP handle
        hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
        if (!hIOCP)
        {
            closesocket(listenerSocket);
            CleanupSslContext();
            WSACleanup();
            return false;
        }

        // Associate listener socket with IOCP (optional)
        HANDLE assoc = CreateIoCompletionPort((HANDLE)listenerSocket, hIOCP, (ULONG_PTR)listenerSocket, 0);
        if (!assoc)
        {
            closesocket(listenerSocket);

            CloseHandle(hIOCP);
            WSACleanup();
            return false;
        }

        // Load AcceptEx function pointer
        GUID acceptex_guid = WSAID_ACCEPTEX;
        DWORD bytes = 0;
        if (WSAIoctl(listenerSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
            &acceptex_guid, sizeof(acceptex_guid),
            &lpfnAcceptEx, sizeof(lpfnAcceptEx), &bytes, NULL, NULL) == SOCKET_ERROR)
        {
            Stop();
            return false;
        }

        // Load GetAcceptExSockaddrs function pointer
        GUID getsockaddrs_guid = WSAID_GETACCEPTEXSOCKADDRS;
        if (WSAIoctl(listenerSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
            &getsockaddrs_guid, sizeof(getsockaddrs_guid),
            &lpfnGetAcceptExSockaddrs, sizeof(lpfnGetAcceptExSockaddrs), &bytes, NULL, NULL) == SOCKET_ERROR)
        {
            Stop();
            return false;
        }

        running = true;

        // Start worker threads = number of CPU cores
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        int threadCount = (int)sysinfo.dwNumberOfProcessors;
        for (int i = 0; i < threadCount; i++)
        {
            if (useSSL) workerThreads.emplace_back(&Server::WorkerThreadSsl, this);
            else workerThreads.emplace_back(&Server::WorkerThread, this);
        }

        // Post initial AcceptEx
        for (int i = 0; i < threadCount * 2; i++)
        { // multiple accepts pending
            if (!PostAccept())
            {
                Stop();
                return false;
            }
        }
        return true;
    }
    void Server::Stop()
    {
        if (!running) return;
        running = false;
        if (listenerSocket != INVALID_SOCKET)
        {
            closesocket(listenerSocket);
            listenerSocket = INVALID_SOCKET;
        }
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            for (SOCKET s : clients)
            {
                if (s != INVALID_SOCKET)
                {
                    ::shutdown(s, SD_BOTH); // Shutdown read/write
                    ::closesocket(s);
                }
            }
            clients.clear();
        }
        for (size_t i = 0; i < workerThreads.size(); i++)
        {
            PostQueuedCompletionStatus(hIOCP, 0, 0, nullptr);
        }
        for (auto& th : workerThreads)
        {
            if (th.joinable()) th.join();
        }
        workerThreads.clear();
        if (hIOCP)
        {
            CloseHandle(hIOCP);
            hIOCP = nullptr;
        }
        CleanupSslContext();
        WSACleanup();
    }
    void Server::MapControllers(Web::RouteConfig* config)
    {
        config->RegisterEndPoints(&Web::Router::Instance());
    }
    void Server::Shutdown()
    {
        if (!running) return;
        running = false;

        // Stop listening
        if (listenerSocket != INVALID_SOCKET)
        {
            closesocket(listenerSocket);
            listenerSocket = INVALID_SOCKET;
        }

        // Wake up worker threads
        for (size_t i = 0; i < workerThreads.size(); i++)
        {
            PostQueuedCompletionStatus(hIOCP, 0, 0, nullptr);
        }

        // Wait all worker threads
        for (auto& th : workerThreads)
        {
            if (th.joinable())
                th.join();
        }
        workerThreads.clear();

        // Close IOCP
        if (hIOCP)
        {
            CloseHandle(hIOCP);
            hIOCP = nullptr;
        }

        // Note: Tidak cleanup SSL dan Winsock agar bisa di-reinit
    }

    bool Server::PostAccept()
    {
        SOCKET acceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (acceptSocket == INVALID_SOCKET)
        {
            return false;
        }

        IOContext* ctx = new IOContext();
        ctx->socket = acceptSocket;
        ctx->operationType = Operation::Accept;

        // Associate socket to IOCP
        if (!CreateIoCompletionPort((HANDLE)acceptSocket, hIOCP, (ULONG_PTR)acceptSocket, 0))
        {
            closesocket(acceptSocket);
            delete ctx;
            return false;
        }

        DWORD bytesReceived = 0;
        BOOL result = lpfnAcceptEx(
            listenerSocket,
            acceptSocket,
            ctx->buffer,
            0,
            sizeof(sockaddr_in) + 16,
            sizeof(sockaddr_in) + 16,
            &bytesReceived,
            &ctx->overlapped);

        if (!result && WSAGetLastError() != ERROR_IO_PENDING)
        {
            closesocket(acceptSocket);
            delete ctx;
            return false;
        }

        return true;
    }
    bool Server::StartReceive(IOContext* ctx)
    {
        ctx->operationType = Operation::Receive;
        ZeroMemory(&ctx->overlapped, sizeof(ctx->overlapped));
        ZeroMemory(&ctx->wsabuf, sizeof(ctx->wsabuf));
        ctx->wsabuf.buf = ctx->buffer;
        ctx->wsabuf.len = sizeof(ctx->buffer);
        DWORD flags = 0;

        int result = WSARecv(ctx->socket, &ctx->wsabuf, 1, NULL, &flags, &ctx->overlapped, NULL);
        return !(result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING);
    }
    void Server::ProcessReceiveAsync(IOContext* ctx)
    {

    }

    bool Server::StartHandshake(IOContext* ctx)
    {
        return false;
    }
    void Server::ProcessHandshakeAsync(IOContext* ctx)
    {
        int ret = SSL_do_handshake(ctx->ssl);
        int err = SSL_get_error(ctx->ssl, ret);

        if (ret == 1)
        {
            StartReceive(ctx); // lanjut ke baca data dari client
            return;
        }

        if (err == SSL_ERROR_WANT_READ)
        {
            // Perlu kirim data ke client dari wbio
            char tempBuf[8192];
            int bytesToSend = BIO_read(ctx->wbio, tempBuf, sizeof(tempBuf));
            if (bytesToSend > 0)
            {
                memcpy(ctx->buffer, tempBuf, bytesToSend);
                ctx->wsabuf.buf = ctx->buffer;
                ctx->wsabuf.len = bytesToSend;
                ctx->operationType = Operation::Send;
                ProcessSendAsync(ctx);
            }
            else
            {
                delete ctx;
                ctx = nullptr;
            }
        }
        else if (err == SSL_ERROR_WANT_WRITE)
        {
            // Biasanya tidak terjadi karena kita pakai BIO memory, tapi tetap ditangani
            // Tidak usah kirim apa-apa dulu, mungkin akan dapat event IO lagi
        }
        else
        {
            ERR_print_errors_fp(stderr); // debug
            delete ctx;
            ctx = nullptr;
        }
    }

    bool Server::StartSend(IOContext* ctx)
    {
        size_t toSend = min(ctx->data.size(), sizeof(ctx->buffer));

        memcpy(ctx->buffer, ctx->data.c_str(), toSend);
        ctx->wsabuf.buf = ctx->buffer;
        ctx->wsabuf.len = static_cast<ULONG>(toSend);
        ctx->operationType = Operation::Send;

        ZeroMemory(&ctx->overlapped, sizeof(ctx->overlapped));
        DWORD sentBytes = 0;
        int res = WSASend(ctx->socket, &ctx->wsabuf, 1, &sentBytes, 0, &ctx->overlapped, NULL);
        return !(res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING);
    }
    bool Server::ProcessSendAsync(IOContext* ctx)
    {
        if (ctx->dataTransfered < ctx->data.size())
        {
            size_t remaining = ctx->data.size() - ctx->dataTransfered;
            size_t toSend = min(remaining, sizeof(ctx->buffer));

            memcpy(ctx->buffer, ctx->data.c_str() + ctx->dataTransfered, toSend);
            ctx->wsabuf.buf = ctx->buffer;
            ctx->wsabuf.len = static_cast<ULONG>(toSend);
            ctx->operationType = Operation::Send;

            ZeroMemory(&ctx->overlapped, sizeof(ctx->overlapped));
            DWORD sentBytes = 0;
            int res = WSASend(ctx->socket, &ctx->wsabuf, 1, &sentBytes, 0, &ctx->overlapped, NULL);
            return !(res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING);
        }
        return false;
    }

    void Server::CleanupSocket(SOCKET s)
    {
        if (s != INVALID_SOCKET)
        {
            closesocket(s); // hanya tutup socket klien

            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.erase(std::remove(clients.begin(), clients.end(), s), clients.end());
        }
    }
    void Server::CleanupContext(IOContext* ctx)
    {
        if (ctx != nullptr)
        {
            delete ctx;
            ctx = nullptr;
        }
    }
    void Server::CleanupSslContext()
    {
        if (sslContext != nullptr)
        {
            SSL_CTX_free(sslContext);
        }
    }
    void Server::DisconnectClient(SOCKET s) {
        CleanupSocket(s);
    }
    void Server::WorkerThread()
    {
        DWORD bytesTransferred;
        ULONG_PTR completionKey;
        LPOVERLAPPED overlapped;

        while (running)
        {
            BOOL success = GetQueuedCompletionStatus(hIOCP, &bytesTransferred, &completionKey, &overlapped, INFINITE);
            IOContext* ctx = (IOContext*)overlapped;

            if (!success)
            {
                if (ctx && ctx->operationType == Operation::Accept)
                {
                    PostAccept();
                }
                CleanupContext(ctx);
                continue;
            }
            if (!ctx) break;
            ctx->lastActive = std::chrono::steady_clock::now();

            switch (ctx->operationType)
            {
            case Operation::Accept:
            {
                setsockopt(ctx->socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&listenerSocket, sizeof(listenerSocket));
                {
                    std::lock_guard<std::mutex> lock(clientsMutex);
                    clients.push_back(ctx->socket);
                }
                if (!StartReceive(ctx)) CleanupContext(ctx);
                PostAccept();
                break;
            }
            case Operation::Receive:
            {
                if (bytesTransferred == 0)
                {
                    CleanupContext(ctx);
                    break;
                }
                ctx->data.append(ctx->buffer, bytesTransferred);
                if (ctx->request == nullptr)
                {
                    size_t headerEnd = ctx->data.find("\r\n\r\n");
                    if (headerEnd != std::string::npos)
                    {
                        std::string rawHeader = ctx->data.substr(0, headerEnd + 4);
                        ctx->request = std::make_unique<Web::HttpRequest>(rawHeader);

                        std::string remaining = ctx->data.substr(headerEnd + 4);
                        ctx->request->body = remaining;

                        ctx->data.clear();
                    }
                }
                else
                {
                    ctx->request->body.append(ctx->data);
                    ctx->data.clear();
                }

                if (ctx->request != nullptr && ctx->request->body.size() < ctx->request->contentLength)
                {
                    if (!StartReceive(ctx)) CleanupContext(ctx);
                    break;
                }

                HandleRequest(ctx);
                if (!StartSend(ctx)) CleanupContext(ctx);
                break;
            }
            case Operation::Send:
            {
                ctx->dataTransfered += bytesTransferred;
                if (!ProcessSendAsync(ctx)) CleanupContext(ctx);
                break;
            }
            default:
                CleanupContext(ctx);
                break;
            }
        }
    }
    void Server::WorkerThreadSsl()
    {
        DWORD bytesTransferred;
        ULONG_PTR completionKey;
        LPOVERLAPPED overlapped;
        while (running)
        {
            BOOL success = GetQueuedCompletionStatus(hIOCP, &bytesTransferred, &completionKey, &overlapped, INFINITE);
            IOContext* ctx = (IOContext*)overlapped;

            if (!success)
            {
                if (ctx && ctx->operationType == Operation::Accept) PostAccept();
                CleanupContext(ctx);
                continue;
            }
            if (!ctx) break;
            ctx->lastActive = std::chrono::steady_clock::now();

            switch (ctx->operationType)
            {
            case Operation::Accept:
            {
                setsockopt(ctx->socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&listenerSocket, sizeof(listenerSocket));
                {
                    std::lock_guard<std::mutex> lock(clientsMutex);
                    clients.push_back(ctx->socket);
                }
                if (!ctx->PrepareSSL(sslContext))
                {
                    CleanupContext(ctx);
                    PostAccept();
                    break;
                }

                int result = SSL_accept(ctx->ssl);
                if (result <= 0)
                {
                    int sslErr = SSL_get_error(ctx->ssl, result);
                    if (sslErr == SSL_ERROR_WANT_READ || sslErr == SSL_ERROR_WANT_WRITE)
                    {
                        //Kirim data TLS ke klien (dari wbio)
                        int pending = BIO_read(ctx->wbio, ctx->buffer, sizeof(ctx->buffer));
                        if (pending > 0)
                        {
                            ctx->data = std::string(ctx->buffer, pending);
                            if (!ProcessSendAsync(ctx))
                            {
                                CleanupContext(ctx);
                                PostAccept();
                            }
                            break;
                        }
                        if (!StartReceive(ctx))
                        {
                            CleanupContext(ctx);
                            PostAccept();
                        }

                        break;
                    }
                    LogSslError(ctx->operationType, sslErr);
                    CleanupContext(ctx);
                    PostAccept();
                    break;
                }

                if (!StartReceive(ctx))
                {
                    CleanupContext(ctx);
                    PostAccept();
                }
                break;
            }
            case Operation::Receive:
            {
                int written = BIO_write(ctx->rbio, ctx->buffer, bytesTransferred); // <- ini raw TLS packet
                if (written <= 0)
                {
                    CleanupContext(ctx);
                    PostAccept();
                    break;
                }

                if (!ctx->HandshakeDone())
                {
                    int result = SSL_accept(ctx->ssl);
                    if (result <= 0)
                    {
                        int sslErr = SSL_get_error(ctx->ssl, result);
                        if (sslErr == SSL_ERROR_WANT_READ || sslErr == SSL_ERROR_WANT_WRITE)
                        {
                            int pending = BIO_read(ctx->wbio, ctx->buffer, sizeof(ctx->buffer));
                            if (pending > 0)
                            {
                                ctx->data = std::string(ctx->buffer, pending);
                                if (!ProcessSendAsync(ctx))
                                {
                                    CleanupContext(ctx);
                                    PostAccept();
                                }
                                break;
                            }
                            if (!StartReceive(ctx))
                            {
                                CleanupContext(ctx);
                                PostAccept();
                            }
                            break;
                        }

                        LogSslError(ctx->operationType, sslErr);
                        CleanupContext(ctx);
                        PostAccept();
                        break;
                    }
                }

                int len = SSL_read(ctx->ssl, ctx->buffer, sizeof(ctx->buffer));
                if (len > 0)
                {
                    std::string raw(ctx->buffer, len);
                    ctx->request = std::make_unique<Web::HttpRequest>(raw);
                    HandleRequest(ctx);

                    int written = SSL_write(ctx->ssl, ctx->data.data(), (int)ctx->data.size());
                    if (written <= 0)
                    {
                        int sslErr = SSL_get_error(ctx->ssl, written);
                        LogSslError(ctx->operationType, sslErr);
                        CleanupContext(ctx);
                        return;
                    }

                    int pending = BIO_read(ctx->wbio, ctx->buffer, sizeof(ctx->buffer));
                    if (pending > 0)
                    {
                        ctx->data = std::string(ctx->buffer, pending);
                        if (!ProcessSendAsync(ctx))
                        {
                            CleanupContext(ctx);
                            break;
                        }
                    }
                    break;
                }
                else
                {
                    int sslErr = SSL_get_error(ctx->ssl, len);
                    if (sslErr == SSL_ERROR_WANT_READ || sslErr == SSL_ERROR_WANT_WRITE)
                    {
                        int pending = BIO_read(ctx->wbio, ctx->buffer, sizeof(ctx->buffer));
                        if (pending > 0)
                        {
                            ctx->data = std::string(ctx->buffer, pending);
                            if (!ProcessSendAsync(ctx))
                            {
                                CleanupContext(ctx);
                                PostAccept();
                            }
                            break;
                        }
                        if (!StartReceive(ctx))
                        {
                            CleanupContext(ctx);
                            PostAccept();
                        }
                        break;
                    }

                    // SSL_read error fatal
                    CleanupContext(ctx);
                    PostAccept();
                    break;
                }
                if (!StartReceive(ctx))
                {
                    CleanupContext(ctx);
                    PostAccept();
                }
                break;
            }
            case Operation::Send:
            {
                int pending = BIO_read(ctx->wbio, ctx->buffer, sizeof(ctx->buffer));
                if (pending > 0)
                {
                    ctx->data = std::string(ctx->buffer, pending);
                    if (!ProcessSendAsync(ctx))
                    {
                        CleanupContext(ctx);
                        PostAccept();
                    }
                    break;
                }
                if (!StartReceive(ctx))
                {
                    CleanupContext(ctx);
                    PostAccept(); 
                }
                break;                
            }
            default:
                DisconnectClient(ctx->socket);
                delete ctx;
                break;
            }
        }
    }
    void Server::LogSslError(Operation operationType, int err)
    {
        std::string message = "";
        switch (operationType)
        {
        case Web::Operation::Accept:
        {
            message = "Operation::Accept:";
            break;
        }
        case Web::Operation::Handshake:
        {
            message = "Operation::Handshake:";
            break;
        }
        case Web::Operation::Receive:
        {
            message = "Operation::Receive:";
            break;
        }
        case Web::Operation::Send:
        {
            message = "Operation::Send:";
            break;
        }
        default:
        {
            break;
        }
        }/*
        std::cerr << message << " SSL_get_error return code: " << std::to_string(err) << std::endl;
        unsigned long e;
        while ((e = ERR_get_error()))
        {
            char msg[256];
            ERR_error_string_n(e, msg, sizeof(msg));
            std::cerr << "OpenSSL Error: " << msg << "\n";
        }*/
    }
    void Server::HandleRequest(IOContext* ctx)
    {
        if (ctx->request->IsFileRequest())
        {
            ctx->data = Web::StaticFileHandler::Serve(ctx->request->url);
            return;
        }

        Web::HttpResponse response = Web::Router::Instance().Handle(*ctx->request);
        ctx->data = response.ToString();
    }
}