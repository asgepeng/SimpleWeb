#include "web.h"
#include <iostream>
#include "initiator.h"
#include <filesystem>

int main()
{
    Web::Server app;
    Web::ControllerRoutes routeConfig;

    app.MapController(routeConfig);

    if (!app.Run()) return 1;

    std::cout << "Server running on https://localhost/\nPress Enter to exit.\n";
    std::string temp;
    std::getline(std::cin, temp);
    app.Stop();
    return 0;
}
