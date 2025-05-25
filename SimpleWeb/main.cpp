#include "web.h"
#include "db.h"

#include <sstream>
#include <iostream>
#include <windows.h>

int wmain(int argc, wchar_t* argv[]) {
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
                << "  <style>\n"
                << "    body { font-family: Arial, sans-serif; margin: 20px; }\n"
                << "    table { border-collapse: collapse; width: 100%; }\n"
                << "    th, td { border: 1px solid #ccc; padding: 8px; text-align: left; }\n"
                << "    th { background-color: #f4f4f4; }\n"
                << "  </style>\n"
                << "</head>\n"
                << "<body>\n";

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

    Web::Server server(8080);
    server.setRouter(&router);
    if (!server.start()) 
    {
        std::cerr << "Server failed to start\n";
        return 1;
    }

    std::wcout << L"Server running on http://localhost:8080/\nPress Enter to exit.\n";
    std::wstring temp;
    std::getline(std::wcin, temp);
    server.stop();
    return 0;
}


