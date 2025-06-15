#include "service.h"

#include <windows.h>

int main()
{
#ifdef _DEBUG
    // Debug mode: tetap bisa run sebagai console app
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

    std::cout << "Server running. Press Enter to stop..." << std::endl;
    std::cin.get();
    server.Stop();
#else
    // Release mode: jalankan sebagai service
    WindowsService service(L"AstroWeb");
    if (!service.Run())
    {
        std::cerr << "Service failed to run!" << std::endl;
        return 1;
    }
#endif

    return 0;
}