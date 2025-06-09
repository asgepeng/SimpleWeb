#include "routing.h"
#include "server.h"
#include "staticfile.h"
#include "appconfig.h"

#include <openssl/err.h>
#include <exception>

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
            return false;
        }

        if (SSL_CTX_use_certificate_file(sslContext, certPath.c_str(), SSL_FILETYPE_PEM) <= 0)
        {
            ERR_print_errors_fp(stderr);
            return false;
        }

        if (SSL_CTX_use_PrivateKey_file(sslContext, keyPath.c_str(), SSL_FILETYPE_PEM) <= 0)
        {
            ERR_print_errors_fp(stderr);
            return false;
        }

        if (!SSL_CTX_check_private_key(sslContext))
        {
            std::cerr << "Private key does not match the public certificate\n";
            return false;
        }

        std::cout << "ssl successfully initialized";
        return true;
    }
    bool Server::Start()
    {
        if (useSSL)
        {
            if (!InitializeSsl())
            {
                return false;
            }
            port = 443;
        }
        else
        {
            if (port == 0) port = 80;
        }

        WSADATA wsaData;
        int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (res != 0)
        {
            return false;
        }

        listenerSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (listenerSocket == INVALID_SOCKET)
        {
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
            WSACleanup();
            return false;
        }

        if (listen(listenerSocket, SOMAXCONN) == SOCKET_ERROR)
        {
            closesocket(listenerSocket);
            WSACleanup();
            return false;
        }

        // Create IOCP handle
        hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
        if (!hIOCP)
        {
            closesocket(listenerSocket);
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
            if (!PostAccept()) {
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

        // 1. Stop listening
        if (listenerSocket != INVALID_SOCKET)
        {
            closesocket(listenerSocket);
            listenerSocket = INVALID_SOCKET;
        }

        // 2. Shutdown all active client connections
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

        // 3. Wake up all worker threads
        for (size_t i = 0; i < workerThreads.size(); i++)
        {
            PostQueuedCompletionStatus(hIOCP, 0, 0, nullptr);
        }

        // 4. Wait all worker threads
        for (auto& th : workerThreads)
        {
            if (th.joinable())
                th.join();
        }
        workerThreads.clear();

        // 5. Release IOCP
        if (hIOCP)
        {
            CloseHandle(hIOCP);
            hIOCP = nullptr;
        }

        // 6. Cleanup SSL
        if (sslContext)
        {
            SSL_CTX_free(sslContext);
            sslContext = nullptr;
        }

        // 7. Cleanup WinSock
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
    void Server::PostReceive(IOContext* ctx)
    {
        ZeroMemory(&ctx->overlapped, sizeof(OVERLAPPED));
        ctx->operationType = Operation::Receive;
        ctx->wsabuf.buf = ctx->buffer;
        ctx->wsabuf.len = sizeof(ctx->buffer);
        DWORD flags = 0;
        int res = WSARecv(ctx->socket, &ctx->wsabuf, 1, nullptr, &flags, &ctx->overlapped, nullptr);
        if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
        {
            DisconnectClient(ctx->socket);
        }
    }
    void Server::PrepareHandshake(IOContext* ctx, int sslError)
    {
        ZeroMemory(&ctx->overlapped, sizeof(OVERLAPPED));

        if (sslError == SSL_ERROR_WANT_READ)
        {
            ctx->operationType = Operation::Handshake;

            ctx->wsabuf.buf = ctx->buffer;
            ctx->wsabuf.len = sizeof(ctx->buffer);
            DWORD flags = 0;

            int res = WSARecv(ctx->socket, &ctx->wsabuf, 1, nullptr, &flags, &ctx->overlapped, nullptr);
            if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
            {
                DisconnectClient(ctx->socket);
            }
        }
        else if (sslError == SSL_ERROR_WANT_WRITE)
        {
            ctx->operationType = Operation::Handshake;
            ctx->wsabuf.buf = ctx->buffer;
            ctx->wsabuf.len = 0;

            int res = WSASend(ctx->socket, &ctx->wsabuf, 1, nullptr, 0, &ctx->overlapped, nullptr);
            if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
            {
                DisconnectClient(ctx->socket);
            }
        }
        else
        {
            DisconnectClient(ctx->socket);
        }
    }

    void Server::SendPendingBIO(IOContext* ctx)
    {
        char outBuffer[4096];
        BIO* wbio = SSL_get_wbio(ctx->ssl);
        while (BIO_pending(wbio) > 0)
        {
            int len = BIO_read(wbio, outBuffer, sizeof(outBuffer));
            if (len <= 0)
                break;

            WSABUF wsabuf;
            wsabuf.buf = outBuffer;
            wsabuf.len = len;

            WSAOVERLAPPED* overlapped = new WSAOVERLAPPED();
            ZeroMemory(overlapped, sizeof(WSAOVERLAPPED));
            DWORD sentBytes = 0;

            int ret = WSASend(ctx->socket, &wsabuf, 1, &sentBytes, 0, overlapped, nullptr);
            if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
            {
                delete overlapped;
                DisconnectClient(ctx->socket);
                break;
            }
        }
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
                    delete ctx;
                    continue;
                }
                else if (ctx) {
                    DisconnectClient(ctx->socket);
                    delete ctx;
                }
                continue;
            }

            if (!ctx)
            {
                // Possibly shutdown signal
                break;
            }

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

                // Prepare for first Receive
                ctx->operationType = Operation::Receive;
                ZeroMemory(&ctx->overlapped, sizeof(ctx->overlapped));
                ZeroMemory(&ctx->wsabuf, sizeof(ctx->wsabuf));
                ctx->wsabuf.buf = ctx->buffer;
                ctx->wsabuf.len = sizeof(ctx->buffer);
                DWORD flags = 0;
                int res = WSARecv(ctx->socket, &ctx->wsabuf, 1, NULL, &flags, &ctx->overlapped, NULL);
                if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
                {
                    DisconnectClient(ctx->socket);
                    delete ctx;
                    PostAccept();
                    break;
                }

                PostAccept(); // Post next Accept
                break;
            }
            case Operation::Receive:
            {
                if (bytesTransferred == 0)
                {
                    DisconnectClient(ctx->socket);
                    delete ctx;
                    break;
                }

                std::string data(ctx->buffer, bytesTransferred);
                if (ctx->request == nullptr)
                {
                    auto headerEnd = data.find("\r\n\r\n");
                    if (headerEnd != std::string::npos)
                    {
                        ctx->request = new Web::HttpRequest(data);
                    }
                }
                else
                {
                    ctx->request->body.append(data);
                }

                if (ctx->request != nullptr && ctx->request->body.size() < ctx->request->contentLength)
                {
                    ZeroMemory(&ctx->overlapped, sizeof(ctx->overlapped));
                    ctx->operationType = Operation::Receive;

                    DWORD flags = 0;
                    DWORD recvBytes = 0;
                    ctx->wsabuf.buf = ctx->buffer;
                    ctx->wsabuf.len = sizeof(ctx->buffer);

                    int res = WSARecv(ctx->socket, &ctx->wsabuf, 1, &recvBytes, &flags, &ctx->overlapped, nullptr);
                    if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
                    {
                        DisconnectClient(ctx->socket);
                        delete ctx;
                    }
                    break;
                }

                std::string response;
                HandleRequest(*ctx->request, response);

                ctx->data = response;
                size_t toSend = min(response.size(), sizeof(ctx->buffer));

                memcpy(ctx->buffer, ctx->data.c_str(), toSend);
                ctx->wsabuf.buf = ctx->buffer;
                ctx->wsabuf.len = static_cast<ULONG>(toSend);
                ctx->operationType = Operation::Send;

                ZeroMemory(&ctx->overlapped, sizeof(ctx->overlapped));
                DWORD sentBytes = 0;
                int res = WSASend(ctx->socket, &ctx->wsabuf, 1, &sentBytes, 0, &ctx->overlapped, NULL);
                if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
                {
                    DisconnectClient(ctx->socket);
                    delete ctx;
                    break;
                }
                break;
            }
            case Operation::Send:
            {
                ctx->dataTransfered += bytesTransferred;
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
                    if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
                    {
                        DisconnectClient(ctx->socket);
                        delete ctx;
                    }
                }
                else
                {
                    DisconnectClient(ctx->socket);
                    delete ctx;
                }
                break;
            }
            default:
                delete ctx;
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
            IOContext* ctx = reinterpret_cast<IOContext*>(overlapped);

            if (!ctx) continue;

            ctx->lastActive = std::chrono::steady_clock::now();

            if (!success)
            {
                if (ctx->operationType == Operation::Accept)
                {
                    PostAccept();
                }

                DisconnectClient(ctx->socket);
                delete ctx;
                continue;
            }

            switch (ctx->operationType)
            {
            case Operation::Accept:
            {
                setsockopt(ctx->socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&listenerSocket, sizeof(listenerSocket));

                {
                    std::lock_guard<std::mutex> lock(clientsMutex);
                    clients.push_back(ctx->socket);
                }

                BIO* bio = BIO_new_socket(ctx->socket, BIO_NOCLOSE);
                SSL_set_bio(ctx->ssl, bio, bio);

                PostAccept();

                int ret = SSL_accept(ctx->ssl);
                if (ret <= 0)
                {
                    int err = SSL_get_error(ctx->ssl, ret);
                    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
                    {
                        std::cout << "Error : if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)" << std::to_string(err) << std::endl;
                        ctx->operationType = Operation::Handshake;
                        PrepareHandshake(ctx, err);
                        break;
                    }

                    DisconnectClient(ctx->socket);
                    delete ctx;
                    break;
                }

                ctx->operationType = Operation::Receive;
                PostReceive(ctx);
                break;
            }

            case Operation::Handshake:
            {
                int ret = SSL_accept(ctx->ssl);
                if (ret <= 0)
                {
                    int err = SSL_get_error(ctx->ssl, ret);
                    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
                    {
                        std::cout << "Error : case Operation::Handshake:" << std::to_string(err) << std::endl;
                        PrepareHandshake(ctx, err);
                        break;
                    }

                    DisconnectClient(ctx->socket);
                    delete ctx;
                    break;
                }

                ctx->operationType = Operation::Receive;
                PostReceive(ctx);
                break;
            }

            case Operation::Receive:
            {
                if (bytesTransferred == 0)
                {
                    DisconnectClient(ctx->socket);
                    delete ctx;
                    break;
                }

                int written = BIO_write(SSL_get_rbio(ctx->ssl), ctx->buffer, bytesTransferred);
                if (written <= 0)
                {
                    DisconnectClient(ctx->socket);
                    delete ctx;
                    break;
                }

                char plaintextBuf[4096];
                int decryptedLen = SSL_read(ctx->ssl, plaintextBuf, sizeof(plaintextBuf));
                if (decryptedLen <= 0)
                {
                    int err = SSL_get_error(ctx->ssl, decryptedLen);
                    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
                    {
                        PostReceive(ctx);
                        break;
                    }

                    DisconnectClient(ctx->socket);
                    delete ctx;
                    break;
                }

                std::string data(plaintextBuf, decryptedLen);
                if (!ctx->request)
                {
                    size_t pos = data.find("\r\n\r\n");
                    if (pos != std::string::npos)
                    {
                        ctx->request = new Web::HttpRequest(data);
                    }
                }
                else
                {
                    ctx->request->body.append(data);
                }

                if (ctx->request && ctx->request->body.size() < ctx->request->contentLength)
                {
                    PostReceive(ctx);
                    break;
                }

                std::string response;
                HandleRequest(*ctx->request, response);
                ctx->data = response;
                ctx->dataTransfered = 0;
                ctx->lastSentPlaintext = 0;

                // Encrypt & send first chunk
                size_t toSend = std::min<size_t>(response.size(), 4096);
                int encryptedLen = SSL_write(ctx->ssl, response.data(), (int)toSend);
                if (encryptedLen <= 0)
                {
                    DisconnectClient(ctx->socket);
                    delete ctx;
                    break;
                }

                ctx->lastSentPlaintext = encryptedLen;

                int bioLen = BIO_read(SSL_get_wbio(ctx->ssl), ctx->buffer, sizeof(ctx->buffer));
                if (bioLen <= 0)
                {
                    DisconnectClient(ctx->socket);
                    delete ctx;
                    break;
                }

                ctx->wsabuf.buf = ctx->buffer;
                ctx->wsabuf.len = bioLen;
                ctx->operationType = Operation::Send;
                ZeroMemory(&ctx->overlapped, sizeof(ctx->overlapped));

                DWORD sentBytes = 0;
                int res = WSASend(ctx->socket, &ctx->wsabuf, 1, &sentBytes, 0, &ctx->overlapped, nullptr);
                if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
                {
                    DisconnectClient(ctx->socket);
                    delete ctx;
                }
                break;
            }

            case Operation::Send:
            {
                ctx->dataTransfered += ctx->lastSentPlaintext;

                if (ctx->dataTransfered < ctx->data.size())
                {
                    size_t remaining = ctx->data.size() - ctx->dataTransfered;
                    size_t toWrite = std::min<size_t>(remaining, 4096);

                    int written = SSL_write(ctx->ssl, ctx->data.data() + ctx->dataTransfered, (int)toWrite);
                    if (written <= 0)
                    {
                        int err = SSL_get_error(ctx->ssl, written);
                        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
                        {
                            PostReceive(ctx); // bisa diganti PostSend(ctx) jika kamu pisahkan IO write
                            break;
                        }

                        DisconnectClient(ctx->socket);
                        delete ctx;
                        break;
                    }

                    ctx->lastSentPlaintext = written;

                    int bioLen = BIO_read(SSL_get_wbio(ctx->ssl), ctx->buffer, sizeof(ctx->buffer));
                    if (bioLen <= 0)
                    {
                        DisconnectClient(ctx->socket);
                        delete ctx;
                        break;
                    }

                    ctx->wsabuf.buf = ctx->buffer;
                    ctx->wsabuf.len = bioLen;
                    ctx->operationType = Operation::Send;
                    ZeroMemory(&ctx->overlapped, sizeof(ctx->overlapped));

                    DWORD sentBytes = 0;
                    int res = WSASend(ctx->socket, &ctx->wsabuf, 1, &sentBytes, 0, &ctx->overlapped, nullptr);
                    if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
                    {
                        DisconnectClient(ctx->socket);
                        delete ctx;
                    }
                }
                else
                {
                    DisconnectClient(ctx->socket);
                    delete ctx;
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
    void Server::HandleRequest(Web::HttpRequest& request, std::string& dataout)
    {
        if (request.IsFileRequest())
        {
            dataout = Web::StaticFileHandler::Serve(request.url);
            return;
        }

        Web::HttpResponse response = Web::Router::Instance().Handle(request);
        dataout = response.ToString();
    }
}