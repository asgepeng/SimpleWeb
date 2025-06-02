// IOCPServer.cpp
#include "iocp.h"
#include <ws2tcpip.h>
#include <iostream>
#include <openssl/err.h>

#define OP_READ      0
#define OP_WRITE     1
#define OP_HANDSHAKE 2

namespace Web
{
    // Struktur data per-handle untuk setiap koneksi client.
    struct PER_HANDLE_DATA
    {
        SOCKET socket;
        SSL* ssl;
    };

    // Struktur data per I/O untuk setiap operasi I/O.
    struct PER_IO_DATA
    {
        OVERLAPPED overlapped;
        WSABUF wsaBuf;
        char buffer[4096];
        DWORD operation;
    };

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
        threadPool.Start(threadCount, hIOCP, WorkerThread);

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

        connManager.CleanupAll();
        sslManager.Cleanup();
        WSACleanup();
        logger.Info("Server stopped.");
    }

    void IOCPServer::AcceptLoop()
    {
        while (running) {
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
            connManager.Add(conn);
        }
    }

    DWORD WINAPI IOCPServer::WorkerThread(LPVOID lpParam)
    {
        HANDLE hIOCP = (HANDLE)lpParam;
        DWORD bytesTransferred;
        PER_HANDLE_DATA* pHandleData = nullptr;
        PER_IO_DATA* pIoData = nullptr;
        DWORD flags = 0;

        while (true) 
        {
            // Deklarasikan variabel flag lokal untuk GetQueuedCompletionStatus.
            flags = 0;
            BOOL result = GetQueuedCompletionStatus(hIOCP, &bytesTransferred, (PULONG_PTR)&pHandleData, (LPOVERLAPPED*)&pIoData, INFINITE);

            if (!result) 
            {
                if (pHandleData == nullptr) continue;
            }
            // Jika tidak ada byte yang ditransfer, berarti koneksi sudah ditutup.
            if (bytesTransferred == 0) 
            {
                std::cout << "Connection closed by client." << std::endl;
                closesocket(pHandleData->socket);
                if (pHandleData->ssl) {
                    SSL_shutdown(pHandleData->ssl);
                    SSL_free(pHandleData->ssl);
                }
                delete pHandleData;
                delete pIoData;
                continue;
            }

            // Tulis data yang diterima ke memory BIO (rbio) agar SSL dapat memproses handshake.
            int written = BIO_write(SSL_get_rbio(pHandleData->ssl), pIoData->buffer, bytesTransferred);
            if (written <= 0) {
                std::cerr << "BIO_write failed." << std::endl;
                closesocket(pHandleData->socket);
                SSL_shutdown(pHandleData->ssl);
                SSL_free(pHandleData->ssl);
                delete pHandleData;
                delete pIoData;
                continue;
            }

            // Jika handshake TLS belum selesai, proses SSL_accept().
            if (!SSL_is_init_finished(pHandleData->ssl)) {
                int ret = SSL_accept(pHandleData->ssl);
                if (ret <= 0) {
                    int sslErr = SSL_get_error(pHandleData->ssl, ret);
                    if (sslErr == SSL_ERROR_WANT_READ || sslErr == SSL_ERROR_WANT_WRITE)
                    {
                        // Jika ada data pada write BIO (wbio) untuk dikirim, coba kirim.
                        int pending = BIO_ctrl_pending(SSL_get_wbio(pHandleData->ssl));
                        if (pending > 0) {
                            char outBuffer[4096];
                            int bytesOut = BIO_read(SSL_get_wbio(pHandleData->ssl), outBuffer, sizeof(outBuffer));
                            if (bytesOut > 0) {
                                WSABUF sendBuf;
                                sendBuf.buf = outBuffer;
                                sendBuf.len = bytesOut;
                                DWORD sentBytes = 0;
                                int sendRet = WSASend(pHandleData->socket, &sendBuf, 1, &sentBytes, 0, NULL, NULL);
                                if (sendRet == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
                                    std::cerr << "WSASend error while sending handshake response." << std::endl;
                                    closesocket(pHandleData->socket);
                                    SSL_shutdown(pHandleData->ssl);
                                    SSL_free(pHandleData->ssl);
                                    delete pHandleData;
                                    delete pIoData;
                                    continue;
                                }
                            }
                        }
                        // Repost operasi recv untuk mengumpulkan sisa data handshake.
                        ZeroMemory(&pIoData->overlapped, sizeof(OVERLAPPED));
                        pIoData->wsaBuf.len = sizeof(pIoData->buffer);
                        pIoData->wsaBuf.buf = pIoData->buffer;
                        pIoData->operation = OP_HANDSHAKE;
                        flags = 0;  // variabel flag lokal
                        int wsaRet = WSARecv(pHandleData->socket, &pIoData->wsaBuf, 1,
                            NULL, &flags, &pIoData->overlapped, NULL);
                        if (wsaRet == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
                            std::cout << "WSARecv error during handshake re-post." << std::endl;
                            closesocket(pHandleData->socket);
                            SSL_shutdown(pHandleData->ssl);
                            SSL_free(pHandleData->ssl);
                            delete pHandleData;
                            delete pIoData;
                            continue;
                        }
                        continue; // Tunggu data handshake selanjutnya.
                    }
                    else
                    {
                        std::cout << "SSL handshake failed. Error code: " << sslErr << std::endl;
                        ERR_print_errors_fp(stderr);
                        closesocket(pHandleData->socket);
                        SSL_shutdown(pHandleData->ssl);
                        SSL_free(pHandleData->ssl);
                        delete pHandleData;
                        delete pIoData;
                        continue;
                    }
                }
                else 
                {
                    // Jika ada data pada write BIO (wbio), kirimkan ke client.
                    int pending = BIO_ctrl_pending(SSL_get_wbio(pHandleData->ssl));
                    if (pending > 0) 
                    {
                        char outBuffer[4096];
                        int bytesOut = BIO_read(SSL_get_wbio(pHandleData->ssl), outBuffer, sizeof(outBuffer));
                        if (bytesOut > 0)
                        {
                            WSABUF sendBuf;
                            sendBuf.buf = outBuffer;
                            sendBuf.len = bytesOut;
                            DWORD sentBytes = 0;
                            int sendRet = WSASend(pHandleData->socket, &sendBuf, 1, &sentBytes, 0, NULL, NULL);
                            if (sendRet == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
                            {
                                std::cerr << "WSASend error while sending handshake data." << std::endl;
                                closesocket(pHandleData->socket);
                                SSL_shutdown(pHandleData->ssl);
                                SSL_free(pHandleData->ssl);
                                delete pHandleData;
                                delete pIoData;
                                continue;
                            }
                        }
                    }
                    std::cout << "SSL handshake succeeded." << std::endl;
                }
                // Setelah handshake selesai, repost operasi untuk menerima data aplikasi.
                ZeroMemory(&pIoData->overlapped, sizeof(OVERLAPPED));
                pIoData->wsaBuf.len = sizeof(pIoData->buffer);
                pIoData->wsaBuf.buf = pIoData->buffer;
                pIoData->operation = OP_READ;
                flags = 0;

                int wsaRet = WSARecv(pHandleData->socket, &pIoData->wsaBuf, 1, NULL, &flags, &pIoData->overlapped, NULL);
                if (wsaRet == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) 
                {
                    std::cout << "WSARecv error after handshake." << std::endl;
                    closesocket(pHandleData->socket);
                    SSL_shutdown(pHandleData->ssl);
                    SSL_free(pHandleData->ssl);
                    delete pHandleData;
                    delete pIoData;
                    continue;
                }
                continue;
            }
            else
            {
                // Jika handshake sudah selesai, proses data aplikasi.
                int n = SSL_read(pHandleData->ssl, pIoData->buffer, sizeof(pIoData->buffer));
                if (n > 0) 
                {
                    std::string request(pIoData->buffer, n);
                    std::cout << "Received " << n << " bytes:\n" << request << std::endl;

                    // Jika request dimulai dengan "GET", berikan respons HTTP yang valid.
                    if (n >= 3 && strncmp(pIoData->buffer, "GET", 3) == 0) 
                    {
                        std::string httpResponse =
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Length: 13\r\n"
                            "Content-Type: text/html\r\n"
                            "Connection: close\r\n"
                            "\r\n"
                            "Hello, world!";
                        int sent = SSL_write(pHandleData->ssl, httpResponse.c_str(), (int)httpResponse.size());
                        if (sent <= 0) 
                        {
                            std::cout << "SSL_write (response) failed." << std::endl;
                        }
                        // Flush data dari write BIO jika ada.
                        int pending = BIO_ctrl_pending(SSL_get_wbio(pHandleData->ssl));
                        if (pending > 0)
                        {
                            char outBuffer[4096];
                            int bytesOut = BIO_read(SSL_get_wbio(pHandleData->ssl), outBuffer, sizeof(outBuffer));
                            if (bytesOut > 0) 
                            {
                                WSABUF sendBuf;
                                sendBuf.buf = outBuffer;
                                sendBuf.len = bytesOut;
                                DWORD sentBytes = 0;
                                int sendRet = WSASend(pHandleData->socket, &sendBuf, 1, &sentBytes, 0, NULL, NULL);
                                if (sendRet == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
                                    std::cerr << "WSASend error while sending final response." << std::endl;
                                }
                            }
                        }
                        std::cout << "Response sent. Closing connection." << std::endl;
                        closesocket(pHandleData->socket);
                        SSL_shutdown(pHandleData->ssl);
                        SSL_free(pHandleData->ssl);
                        delete pHandleData;
                        delete pIoData;
                        continue;
                    }
                    else {
                        // Jika bukan GET, misalnya echo (atau proses lain).
                        int sent = SSL_write(pHandleData->ssl, pIoData->buffer, n);
                        if (sent <= 0)
                        {
                            std::cout << "SSL_write failed." << std::endl;
                        }
                    }
                    // Repost operasi baca.
                    ZeroMemory(&pIoData->overlapped, sizeof(OVERLAPPED));
                    pIoData->wsaBuf.len = sizeof(pIoData->buffer);
                    pIoData->wsaBuf.buf = pIoData->buffer;
                    pIoData->operation = OP_READ;
                    flags = 0;
                    int wsaRet = WSARecv(pHandleData->socket, &pIoData->wsaBuf, 1, NULL, &flags, &pIoData->overlapped, NULL);
                    if (wsaRet == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
                    {
                        std::cout << "WSARecv error after read." << std::endl;
                        closesocket(pHandleData->socket);
                        SSL_shutdown(pHandleData->ssl);
                        SSL_free(pHandleData->ssl);
                        delete pHandleData;
                        delete pIoData;
                        continue;
                    }
                }
                else
                {
                    std::cout << "SSL_read failed or connection closed." << std::endl;
                    closesocket(pHandleData->socket);
                    SSL_shutdown(pHandleData->ssl);
                    SSL_free(pHandleData->ssl);
                    delete pHandleData;
                    delete pIoData;
                    continue;
                }
            }
        } // end while
        return 0;
    }
}