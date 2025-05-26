#include "web.h"
#include "db.h"

#include <sstream>
#include <iostream>
#include <windows.h>
#include <filesystem>

const std::string staticDirectory = "E:\\PointOfSale\\SimpleWeb\\x64\\Release\\wwwroot\\";
int wmain(int argc, wchar_t* argv[]) {
    std::cout << "Current path: " << std::filesystem::current_path() << std::endl;
    Web::Router router;
    router.addRoute("/", []() 
        {
            Data::DbClient db;
            std::ostringstream oscontent;
            oscontent << "<!DOCTYPE html>\n"
                << "<html lang=\"en\">\n"
                << "<head>\n"
                << "  <meta charset=\"UTF-8\">\n"
                << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                << "  <title>TESTING</title>\n"
                << "  <link href=\"/assets/main.min.css\" rel=\"stylesheet\">"
                << "</head>\n"
                << "<body><img src=\"/images/php.png\"/>\n";

            if (db.Connect()) 
            {
                db.ExecuteHtmlTable("SELECT * FROM categories", oscontent);
                db.Disconnect();
            }
            oscontent << "</body></html>";

            std::string html = oscontent.str();            

            std::ostringstream oss;
            oss << "HTTP/1.1 200 OK\r\n"
                << "Content-Type: text/html\r\n"
                << "Content-Length: " << html.size() << "\r\n"
                << "Connection: close\r\n\r\n"
                << html;
            return oss.str();
        });
    router.addRoute("/home", []() 
        {
            const char* body = R"({"HOME"})";
            std::ostringstream oss;
            oss << "HTTP/1.1 200 OK\r\n"
                << "Content-Type: application/json\r\n"
                << "Content-Length: " << strlen(body) << "\r\n\r\n"
                << body;
            return oss.str();
        });

    Web::Server server(80);
    server.setRouter(&router);
    if (!server.start()) 
    {
        std::cerr << "Server failed to start\n";
        return 1;
    }

    std::wcout << L"Server running on http://localhost/\nPress Enter to exit.\n";
    std::wstring temp;
    std::getline(std::wcin, temp);
    server.stop();
    return 0;
}


