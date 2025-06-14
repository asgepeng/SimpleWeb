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
    bool Server::PostReceive(IOContext* ctx)
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
    bool Server::PostSend(IOContext* ctx)
    {
        if (ctx->dataTransfered < ctx->sendBuffer.size())
        {
            size_t remaining = ctx->sendBuffer.size() - ctx->dataTransfered;
            size_t toSend = min(remaining, sizeof(ctx->buffer));

            memcpy(ctx->buffer, ctx->sendBuffer.c_str() + ctx->dataTransfered, toSend);
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
    void Server::CleanupContext(IOContext* ctx)
    {
        if (ctx->socket != INVALID_SOCKET)
        {
            closesocket(ctx->socket);

            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.erase(std::remove(clients.begin(), clients.end(), ctx->socket), clients.end());
            ctx->socket = INVALID_SOCKET;
        }

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
                if (!PostReceive(ctx)) CleanupContext(ctx);
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
                ctx->receiveBuffer.append(ctx->buffer, bytesTransferred);
                if (ctx->request == nullptr)
                {
                    size_t headerEnd = ctx->receiveBuffer.find("\r\n\r\n");
                    if (headerEnd != std::string::npos)
                    {
                        ctx->request = new Web::HttpRequest(ctx->receiveBuffer);
                        ctx->receiveBuffer.clear();
                    }
                }
                else
                {
                    ctx->request->body.append(ctx->receiveBuffer);
                    ctx->receiveBuffer.clear();
                }

                if (!ctx->ReceiveComplete())
                {
                    if (!PostReceive(ctx)) CleanupContext(ctx);
                    break;
                }

                HandleRequest(ctx);
                if (!PostSend(ctx)) CleanupContext(ctx);
                break;
            }
            case Operation::Send:
            {
                ctx->dataTransfered += bytesTransferred;
                if (!PostSend(ctx)) CleanupContext(ctx);
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
                        int pending = BIO_read(ctx->wbio, ctx->buffer, sizeof(ctx->buffer));
                        if (pending > 0)
                        {
                            ctx->sendBuffer = std::string(ctx->buffer, pending);
                            if (!PostSend(ctx))
                            {
                                CleanupContext(ctx);
                                PostAccept();
                            }
                            break;
                        }
                        if (!PostReceive(ctx))
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

                if (!PostReceive(ctx))
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
                                ctx->sendBuffer = std::string(ctx->buffer, pending);
                                if (!PostSend(ctx))
                                {
                                    CleanupContext(ctx);
                                    PostAccept();
                                }
                                break;
                            }
                            if (!PostReceive(ctx))
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
                    if (ctx->request == nullptr)
                    {
                        ctx->receiveBuffer.append(std::string(ctx->buffer, len));
                        size_t headerEnd = ctx->receiveBuffer.find("\r\n\r\n");
                        if (headerEnd != std::string::npos)
                        {
                            ctx->request = new Web::HttpRequest(ctx->receiveBuffer);
                            ctx->receiveBuffer.clear();
                        }
                        else
                        {
                            if (!PostReceive(ctx))
                            {
                                CleanupContext(ctx);
                                PostAccept();
                            }
                            break;
                        }
                    }
                    else
                    {
                        ctx->request->body.append(std::string(ctx->buffer, len));
                    }         

                    if (!ctx->ReceiveComplete())
                    {
                        if (!PostReceive(ctx))
                        {
                            CleanupContext(ctx);
                            PostAccept();
                        }
                        break;
                    }                    
         
                    HandleRequest(ctx);
                    if (ctx->request)
                    {
                        std::cout << ctx->request->method << std::endl;
                        std::cout << ctx->request->url << std::endl;
                    }

                    int written = SSL_write(ctx->ssl, ctx->sendBuffer.data(), (int)ctx->sendBuffer.size());
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
                        ctx->sendBuffer = std::string(ctx->buffer, pending);
                        if (!PostSend(ctx))
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
                            ctx->sendBuffer = std::string(ctx->buffer, pending);
                            if (!PostSend(ctx))
                            {
                                CleanupContext(ctx);
                                PostAccept();
                            }
                            break;
                        }
                        if (!PostReceive(ctx))
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
                if (!PostReceive(ctx))
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
                    ctx->sendBuffer = std::string(ctx->buffer, pending);
                    if (!PostSend(ctx))
                    {
                        CleanupContext(ctx);
                        PostAccept();
                    }
                    break;
                }
                if (!PostReceive(ctx))
                {
                    CleanupContext(ctx);
                    PostAccept(); 
                }
                break;                
            }
            default:
                CleanupContext(ctx);
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
            ctx->sendBuffer = Web::StaticFileHandler::Serve(ctx->request->url);
            delete ctx->request;
            ctx->request = nullptr;
            return;
        }

        Web::HttpResponse response = Web::Router::Instance().Handle(*ctx->request);
        delete ctx->request;
        ctx->request = nullptr;
        ctx->sendBuffer = response.ToString();
    }
}