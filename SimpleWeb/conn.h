#pragma once
#include "request.h"

#include <WinSock2.h>
#include <chrono>

namespace Web
{
    using namespace std::chrono;

    enum OperationType { Accept, Receive, Send };
	class Connection
	{
    public:
        OVERLAPPED overlapped = {};
        WSABUF wsabuf;
        SOCKET socket = INVALID_SOCKET;
        OperationType operationType;
        char buffer[8192]{};
        steady_clock::time_point lastActive;

        std::string receiveBuffer = "";
        std::string sendBuffer = "";

        size_t lastSentPlaintext = 0;
        size_t dataTransfered = 0;

        HttpRequest* GetRequest();
        bool TryCreateRequest();
        bool ReceiveComplete();

        Connection(SOCKET connectedSocket);
        ~Connection();
    private:
        Web::HttpRequest* request = nullptr;
	};
}

