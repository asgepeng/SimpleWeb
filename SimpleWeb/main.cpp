#include "web.h"
#include <iostream>
#include "initiator.h"
#include <filesystem>

int main()
{
    std::cout << "Current Directory: "
        << std::filesystem::current_path() << std::endl;

    Web::Server app;
    Web::ControllerRoutes routing;

    app.MapController(routing);

    if (!app.Run())
    {
        return 1;
    }

    std::cout << "Server running on http://localhost/\nPress Enter to exit.\n";
    std::string temp;
    std::getline(std::cin, temp);
    app.Stop();
    return 0;
}
