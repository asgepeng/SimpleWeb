#include "server.h"
#include "initiator.h"
#include "layoutmanager.h"
#include "appconfig.h"
#include "dbinitializer.h"

int main() 
{
    Data::DbInitializer::Initialize();
    LayoutManager::Load();
    Configuration::LoadFromFile("config.ini");
    bool useSSL = Configuration::GetBool("UseSSL", false);

    Web::Server server;
    ControllerRoutes routeConfig;

    server.UseSSL(useSSL);
    server.MapControllers(&routeConfig);

    if (!server.Start())
    {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    std::string endpoint = server.UseSSL() ? "https://localhost" : "http://localhost";
    ShellExecuteA(NULL, "open", "msedge", endpoint.c_str(), NULL, SW_SHOWNORMAL);
    std::cout << "Press Enter to stop server..." << std::endl;
    std::cin.get();

    server.Stop();
    return 0;
}