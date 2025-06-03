#include "crypto.h"
#include "layoutmanager.h"
#include "dbinitializer.h"
#include "appconfig.h"
#include "iocp.h"

#include <iostream> // untuk std::getline

using namespace Security::Encryption;
int main()
{
    // Contoh data token
    std::string token = "1~2025-06-06~192.168.1.100";
    std::string cipherText = Security::Encryption::AESEncrypt(token);
    std::cout << "encrypted: " << cipherText << std::endl;
    std::string decrypted = Security::Encryption::AESDecrypt(cipherText);
    std::cout << "decrypted: " << decrypted << std::endl;

    Configuration::LoadFromFile("config.ini");
    Configuration::Print();

    Data::DbInitializer::Initialize();
    LayoutManager::Load();

    Web::IOCPServer server;
    ControllerRoutes routeConfig;

    server.MapController(routeConfig);
    server.Initialize("C:\\Windows\\System32\\server.crt", "C:\\Windows\\System32\\server.key", 443);
    server.Run();

    std::cout << "Tekan Enter untuk menghentikan server..." << std::endl;
    std::string dummy;
    std::getline(std::cin, dummy);  // Perbaikan di sini

    server.Stop();
    return 0;
}

/*
#include "service.h"
#include "layoutmanager.h"

int main()
{
    LayoutManager::Load();
    const wchar_t* serviceName = L"SimpleWeb";
    WindowsService service((LPWSTR)serviceName);
    return service.Run() ? 0 : 1;
}
*/