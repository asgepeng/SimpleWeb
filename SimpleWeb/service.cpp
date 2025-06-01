#pragma once
#include "initiator.h"
#include "web.h"
#include "service.h"
#include <strsafe.h>
#include <string>

// Deklarasi variabel global yang digunakan oleh service
const wchar_t* g_ServiceName = L"SimpleWeb";
SERVICE_STATUS_HANDLE g_StatusHandle = nullptr;
SERVICE_STATUS g_ServiceStatus = {};
HANDLE g_ServiceStopEvent = nullptr;
HANDLE g_ServerThread = nullptr;
Web::Server* g_Server = nullptr;

//
// Kelas WindowsService
//
WindowsService::WindowsService(const wchar_t* serviceName) {
    // Pastikan nama service di-set dengan benar
    g_ServiceName = serviceName;
}

WindowsService::~WindowsService() {
    if (g_Server)
        delete g_Server;
}

bool WindowsService::Run() {
    SERVICE_TABLE_ENTRY ServiceTable[] = {
        { (LPWSTR)g_ServiceName, WindowsService::ServiceMain },
        { nullptr, nullptr }
    };

    if (!StartServiceCtrlDispatcher(ServiceTable))
    {
        DWORD error = GetLastError();
        std::string errMsg = "StartServiceCtrlDispatcher gagal, error: " + std::to_string(error) + "\n";
        OutputDebugStringA(errMsg.c_str());
        return false;
    }
    return true;
}

VOID WINAPI WindowsService::ServiceMain(DWORD argc, LPTSTR* argv) {
    OutputDebugStringA("ServiceMain dipanggil\n");

    g_StatusHandle = RegisterServiceCtrlHandler(g_ServiceName, WindowsService::ServiceCtrlHandler);
    if (!g_StatusHandle) {
        OutputDebugStringA("RegisterServiceCtrlHandler gagal\n");
        return;
    }

    // Inisialisasi struktur status
    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    // Saat start pending, tidak menerima control apapun
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 1;
    g_ServiceStatus.dwWaitHint = 3000;

    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
    OutputDebugStringA("Service is starting...\n");

    // Buat event untuk menghentikan service
    g_ServiceStopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!g_ServiceStopEvent)
    {
        OutputDebugStringA("Failed to create stop event\n");
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        return;
    }

    // Update status ke RUNNING agar service controller tahu service sudah aktif
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwCheckPoint = 0;
    g_ServiceStatus.dwWaitHint = 0;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    OutputDebugStringA("Service is now running.\n");

    // Buat thread untuk menjalankan server
    g_ServerThread = CreateThread(nullptr, 0, WindowsService::ServiceWorkerThread, nullptr, 0, nullptr);
    if (!g_ServerThread) 
    {
        OutputDebugStringA("Failed to create service thread\n");
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        return;
    }

    // Loop untuk menjaga service tetap berjalan
    while (WaitForSingleObject(g_ServiceStopEvent, 1000) != WAIT_OBJECT_0) 
    {
        OutputDebugStringA("Service masih berjalan...\n");
    }

    OutputDebugStringA("Service thread exited. Cleaning up...\n");

    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwCheckPoint = 0;
    g_ServiceStatus.dwWaitHint = 0;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    if (g_ServiceStopEvent)
        CloseHandle(g_ServiceStopEvent);
    if (g_ServerThread)
        CloseHandle(g_ServerThread);
}

VOID WINAPI WindowsService::ServiceCtrlHandler(DWORD ctrlCode) {
    switch (ctrlCode) {
    case SERVICE_CONTROL_STOP:
        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        SetEvent(g_ServiceStopEvent);
        break;
    }
}

DWORD WINAPI WindowsService::ServiceWorkerThread(LPVOID lpParam) {
    OutputDebugStringA("Memulai ServiceWorkerThread\n");

    WindowsService::StartServer();

    // Tetap jaga agar thread tidak keluar sebelum event stop di-trigger
    while (WaitForSingleObject(g_ServiceStopEvent, 1000) != WAIT_OBJECT_0) {
        OutputDebugStringA("Menjaga service tetap berjalan pada ServiceWorkerThread...\n");
    }

    WindowsService::StopServer();
    return 0;
}

void WindowsService::StartServer() {
    OutputDebugStringA("Memulai server...\n");

    g_Server = new Web::Server();

    Web::ControllerRoutes routes;
    g_Server->MapController(routes);

    // Ganti port jika perlu (misalnya 8080) untuk memastikan port 443 tidak dipakai oleh proses lain
    if (!g_Server->Run(443)) {
        OutputDebugStringA("[SimpleWeb] Failed to start server on port 443\n");
        SetEvent(g_ServiceStopEvent);
    }
    else {
        OutputDebugStringA("[SimpleWeb] Server started on port 443\n");
    }
}

void WindowsService::StopServer() {
    if (g_Server) {
        OutputDebugStringA("[SimpleWeb] Stopping server...\n");
        g_Server->Stop();
        OutputDebugStringA("[SimpleWeb] Server stopped.\n");
    }
}