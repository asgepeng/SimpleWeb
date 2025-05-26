#include "web.h"
#include "db.h"
#include "mvc.h"
#include "home.h"
#include "product.h"

#include <sstream>
#include <iostream>
#include <windows.h>
#include <filesystem>



int wmain(int argc, wchar_t* argv[]) {
    Web::Router router;
    router.addRoute("GET", "/", [](Web::HttpContext& context) {
        HomeController home(context);
        return home.Index().ToString();
        });
    router.addRoute("GET", "/products", [](Web::HttpContext& context) {
        ProductController product(context);
        return product.Index().ToString();
        });
    router.addRoute("GET", "/products/edit/{id}", [](Web::HttpContext& context) {
        ProductController p(context);
        return p.Edit(std::stoi(context.RouteData.at("id"))).ToString();
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


