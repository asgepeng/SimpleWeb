#include "web.h"
#include <iostream>
#include "initiator.h"

int main()
{
    Web::Server app;
    Web::ControllerRoutes routing;

    app.UseController(routing);

    if (!app.Run(80))
    {
        std::cerr << "Server failed to start\n";
        return 1;
    }

    std::cout << "Server running on http://localhost/\nPress Enter to exit.\n";
    std::string temp;
    std::getline(std::cin, temp);
    app.Stop();
    return 0;
}
