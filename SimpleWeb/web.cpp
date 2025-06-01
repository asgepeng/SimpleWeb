#include "web.h"
#include "db.h"
#include "staticfile.h"
#include "threadpool.h"

namespace Web
{
    Server::Server() : listener(INVALID_SOCKET), isrun(false), router(std::make_unique<Router>()) {}

    bool Server::Run(const unsigned short port)
    {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return false;

        listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listener == INVALID_SOCKET) return false;

        sockaddr_in service{};
        service.sin_family = AF_INET;
        service.sin_addr.s_addr = INADDR_ANY;
        service.sin_port = htons(port);

        if (bind(listener, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) return false;
        if (listen(listener, SOMAXCONN) == SOCKET_ERROR) return false;

        InitializeSSL();

        isrun = true;
        threads.emplace_back(&Server::AcceptLoop, this);
        return true;
    }

    void Server::Stop()
    {
        isrun = false;

        if (sslCtx)
        {
            SSL_CTX_free(sslCtx);
            sslCtx = nullptr;
        }

        EVP_cleanup();
        closesocket(listener);
        WSACleanup();
    }
    void Server::InitializeSSL()
    {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();

        sslCtx = SSL_CTX_new(TLS_server_method());
        if (!sslCtx) 
        {
            ERR_print_errors_fp(stderr);
            exit(1);
        }

        if (SSL_CTX_use_certificate_file(sslCtx, "server.crt", SSL_FILETYPE_PEM) <= 0 ||
            SSL_CTX_use_PrivateKey_file(sslCtx, "server.key", SSL_FILETYPE_PEM) <= 0)
        {
            ERR_print_errors_fp(stderr);
            exit(1);
        }
    }

    void Server::AcceptLoop()
    {
        ThreadPool pool(std::thread::hardware_concurrency());

        while (isrun)
        {
            SOCKET client = accept(listener, nullptr, nullptr);
            if (client == INVALID_SOCKET) continue;

            pool.enqueue([this, client] {
                Receive(client);
                });
        }
    }

    void Server::Receive(SOCKET clientSocket)
    {
        SSL* ssl = SSL_new(sslCtx);
        SSL_set_fd(ssl, (int)clientSocket);

        if (SSL_accept(ssl) <= 0)
        {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            closesocket(clientSocket);
            return;
        }

        std::string data;
        char buffer[1024];
        int received;

        while ((received = SSL_read(ssl, buffer, sizeof(buffer))) > 0)
        {
            data.append(buffer, received);
            if (data.find("\r\n\r\n") != std::string::npos) break;
        }

        if (received <= 0)
        {
            SSL_shutdown(ssl);
            SSL_free(ssl);
            closesocket(clientSocket);
            return;
        }

        HttpRequest request(data.c_str());
        if (!StaticFileHandler::TryHandleRequest(request, ssl))
        {
            HttpResponse response = router->Handle(request);
            Send(ssl, response);
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        closesocket(clientSocket);
    }
    void Server::Send(SSL* ssl, const HttpResponse& response)
    {
        std::string content = response.ToString();
        SSL_write(ssl, content.c_str(), (int)content.size());
    }
}
