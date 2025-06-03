#include "routing.h"
#include "iocp.h"
#include "staticfile.h"

#include <ws2tcpip.h>
#include <iostream>
#include <openssl/err.h>

namespace Web
{
    void CleanupConnection(Connection* conn, Connection::IOData* pIoData)
    {
        if (conn)
        {
            conn->Cleanup();
            delete conn;
        }
        if (pIoData) delete pIoData;
    }
    void RepostRecv(Connection* conn, Connection::IOData* pIoData, int opType) 
    {
        ZeroMemory(&pIoData->overlapped, sizeof(OVERLAPPED));
        pIoData->wsaBuf.len = sizeof(pIoData->buffer);
        pIoData->wsaBuf.buf = pIoData->buffer;
        pIoData->operation = opType;
        DWORD flags = 0;
        int wsaRet = WSARecv(conn->GetSocket(), &pIoData->wsaBuf, 1, NULL, &flags, &pIoData->overlapped, NULL);
        if (wsaRet == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
            CleanupConnection(conn, pIoData);
    }
    bool SendPendingSSLData(Connection* conn) 
    {
        int pending = BIO_ctrl_pending(SSL_get_wbio(conn->GetSSL()));
        if (pending <= 0) return true;

        char outBuffer[4096];
        int bytesOut = BIO_read(SSL_get_wbio(conn->GetSSL()), outBuffer, sizeof(outBuffer));
        if (bytesOut <= 0) return false;

        WSABUF sendBuf = { static_cast<ULONG>(bytesOut), outBuffer };
        DWORD sentBytes = 0;
        int sendRet = WSASend(conn->GetSocket(), &sendBuf, 1, &sentBytes, 0, NULL, NULL);
        return !(sendRet == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING);
    }
    bool TryFlushBIO(Connection* conn)
    {
        BIO* writeBio = SSL_get_wbio(conn->GetSSL());
        char outBuffer[4096];

        while (BIO_ctrl_pending(writeBio) > 0)
        {
            int bytesOut = BIO_read(writeBio, outBuffer, sizeof(outBuffer));
            if (bytesOut <= 0) return false;

            WSABUF sendBuf;
            sendBuf.buf = outBuffer;
            sendBuf.len = bytesOut;
            DWORD sentBytes = 0;
            int ret = WSASend(conn->GetSocket(), &sendBuf, 1, &sentBytes, 0, nullptr, nullptr);
            if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) return false;
        }

        return true;
    }
    bool TrySSLWrite(Connection* conn, const std::string& data)
    {
        int sent = SSL_write(conn->GetSSL(), data.c_str(), static_cast<int>(data.size()));
        if (sent <= 0)
        {
            int sslErr = SSL_get_error(conn->GetSSL(), sent);
            std::cerr << "SSL_write error: " << sslErr << std::endl;
            return false;
        }
        return true;
    }
    DWORD WINAPI WorkerDoWork(HANDLE hIOCP) 
    {
        while (true)
        {
            DWORD bytesTransferred = 0;
            Connection* conn = nullptr;
            Connection::IOData* pIoData = nullptr;
            DWORD flags = 0;

            BOOL result = GetQueuedCompletionStatus(hIOCP, &bytesTransferred, (PULONG_PTR)&conn, (LPOVERLAPPED*)&pIoData, INFINITE);
            if (!result && conn == nullptr) continue;
            if (bytesTransferred == 0)
            {
                CleanupConnection(conn, pIoData);
                continue;
            }

            int written = BIO_write(SSL_get_rbio(conn->GetSSL()), pIoData->buffer, bytesTransferred);
            if (written <= 0)
            {
                CleanupConnection(conn, pIoData);
                continue;
            }

            if (!conn->HandshakeFinished()) 
            {
                int sslErr = 0;
                if (!conn->Accept(sslErr)) 
                {
                    if (sslErr == SSL_ERROR_WANT_READ || sslErr == SSL_ERROR_WANT_WRITE)
                    {
                        if (!SendPendingSSLData(conn)) 
                        {
                            CleanupConnection(conn, pIoData);
                            continue;
                        }
                        RepostRecv(conn, pIoData, Connection::OP_HANDSHAKE);
                        continue;
                    }
                    else
                    {
                        CleanupConnection(conn, pIoData);
                        continue;
                    }
                }
                else
                {
                    if (!SendPendingSSLData(conn)) 
                    {
                        CleanupConnection(conn, pIoData);
                        continue;
                    }
                    RepostRecv(conn, pIoData, Connection::OP_READ);
                    continue;
                }
            }
            else
            {
                int ret = SSL_read(conn->GetSSL(), pIoData->buffer, sizeof(pIoData->buffer));
                if (ret <= 0)
                {
                    int sslErr = SSL_get_error(conn->GetSSL(), ret);
                    if (sslErr == SSL_ERROR_WANT_READ || sslErr == SSL_ERROR_WANT_WRITE)
                    {
                        RepostRecv(conn, pIoData, Connection::OP_READ);
                        continue;
                    }
                    CleanupConnection(conn, pIoData);
                    continue;
                }

                std::string d(pIoData->buffer, ret);
                std::cout << d << std::endl;
                HttpRequest request(d.c_str());
                std::string httpResponse;

                if (request.IsFileRequest())
                {
                    httpResponse = StaticFileHandler::Serve(request.url);
                }
                else
                {
                    HttpResponse response = Router::Instance().Handle(request);
                    httpResponse = response.ToString();
                }

                if (!TrySSLWrite(conn, httpResponse)) continue;
                if (!TryFlushBIO(conn)) continue;

                CleanupConnection(conn, pIoData);
                continue;
            }
        }
    }


    IOCPServer::IOCPServer() : listenSocket(INVALID_SOCKET), hIOCP(NULL), running(false) {}
    IOCPServer::~IOCPServer() 
    {
        Stop();
    }
    bool IOCPServer::Initialize(const std::string& certFile, const std::string& keyFile, int port)
    {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData)) 
        {
            logger.Error("WSAStartup failed.");
            return false;
        }

        if (!sslManager.Initialize(certFile, keyFile))
        {
            logger.Error("SSL initialization failed.");
            return false;
        }

        listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
        if (listenSocket == INVALID_SOCKET)
        {
            logger.Error("Failed to create listen socket.");
            return false;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        if (bind(listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
        {
            logger.Error("Bind failed.");
            return false;
        }

        if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
        {
            logger.Error("Listen failed.");
            return false;
        }

        hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
        if (!hIOCP) 
        {
            logger.Error("CreateIoCompletionPort failed.");
            return false;
        }

        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        int threadCount = sysInfo.dwNumberOfProcessors * 8;
        threadPool.Start(threadCount, hIOCP, WorkerDoWork);

        return true;
    }

    void IOCPServer::Run()
    {
        running = true;
        logger.Info("Server is running.");
        AcceptLoop();
    }

    void IOCPServer::Stop()
    {
        running = false;
        threadPool.Stop();

        if (listenSocket != INVALID_SOCKET)
        {
            closesocket(listenSocket);
            listenSocket = INVALID_SOCKET;
        }
        if (hIOCP)
        {
            CloseHandle(hIOCP);
            hIOCP = NULL;
        }

        sslManager.Cleanup();
        WSACleanup();
        logger.Info("Server stopped.");
    }

    void IOCPServer::MapController(ControllerRoutes rounteConfig)
    {
        rounteConfig.RegisterEndPoints(&Router::Instance());
    }

    void IOCPServer::AcceptLoop()
    {
        while (running)
        {
            SOCKET clientSocket = accept(listenSocket, NULL, NULL);
            if (clientSocket == INVALID_SOCKET) 
            {
                logger.Warn("Accept failed.");
                continue;
            }

            u_long nonBlocking = 1;
            ioctlsocket(clientSocket, FIONBIO, &nonBlocking);

            Connection* conn = new Connection(clientSocket, sslManager.GetContext());
            if (!conn->StartHandshake(hIOCP)) 
            {
                conn->Cleanup();
                delete conn;
                continue;
            }
            std::cout << "incoming from: " << conn->GetIPAddress() << std::endl;
            //connManager.Add(conn);
        }
    }
}