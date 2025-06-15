#include "conn.h"

namespace Web
{
    HttpRequest* Web::Connection::GetRequest()
    {
        return request;
    }

    bool Connection::TryCreateRequest()
    {
        size_t headerEnd = receiveBuffer.find("\r\n\r\n");
        if (headerEnd == std::string::npos) return false;

        request = new HttpRequest(receiveBuffer);
        receiveBuffer.clear();
        return true;
    }

    bool Connection::ReceiveComplete()
    {
        return (request != nullptr && request->body.size() == request->contentLength);
    }

    Connection::Connection(SOCKET connectedSocket) : socket(connectedSocket), request(nullptr), operationType(OperationType::Accept)
    {
        wsabuf.buf = buffer;
        wsabuf.len = sizeof(buffer);
        lastActive = steady_clock::now();
    }

    Connection::~Connection()
    {
        if (socket != INVALID_SOCKET) closesocket(socket);
        if (request != nullptr)
        {
            delete request;
            request = nullptr;
        }
    }
}
