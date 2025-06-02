#include "reqrouter.h"

std::string Web::RequestRouter::RouteRequest(const std::string& request)
{
    if (request.starts_with("GET")) {
        return "HTTP/1.1 200 OK\r\n"
            "Content-Length: 13\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "\r\n\r\n"
            "Hello, world!";
    }
    return "HTTP/1.1 400 Bad Request\r\n"
        "Content-Length: 11\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "\r\n\r\n"
        "Bad Request";
}
