#pragma once
#include "initiator.h"
#include "service.h"
#include "layoutmanager.h"
#include "appconfig.h"
#include "dbinitializer.h"

#include <strsafe.h>
#include <string>

WindowsService::WindowsService(const wchar_t* serviceName)
{
    g_ServiceName = serviceName;
}

WindowsService::~WindowsService()
{
    if (server) delete server;
}

bool WindowsService::Run() 
{
    SERVICE_TABLE_ENTRY ServiceTable[] = {
        { (LPWSTR)g_ServiceName, WindowsService::ServiceMain },
        { nullptr, nullptr }
    };

    if (!StartServiceCtrlDispatcher(ServiceTable)) return false;
    return true;
}

VOID WINAPI WindowsService::ServiceMain(DWORD argc, LPTSTR* argv)
{
    g_StatusHandle = RegisterServiceCtrlHandler(g_ServiceName, WindowsService::ServiceCtrlHandler);
    if (!g_StatusHandle) 
    {
        return;
    }

    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 1;
    g_ServiceStatus.dwWaitHint = 3000;

    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    g_ServiceStopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!g_ServiceStopEvent)
    {
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        return;
    }

    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwCheckPoint = 0;
    g_ServiceStatus.dwWaitHint = 0;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    g_ServerThread = CreateThread(nullptr, 0, WindowsService::ServiceWorkerThread, nullptr, 0, nullptr);
    if (!g_ServerThread) 
    {
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        return;
    }

    while (WaitForSingleObject(g_ServiceStopEvent, 1000) != WAIT_OBJECT_0) 
    {

    }

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

VOID WINAPI WindowsService::ServiceCtrlHandler(DWORD ctrlCode)
{
    switch (ctrlCode) 
    {
    case SERVICE_CONTROL_STOP:
        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        SetEvent(g_ServiceStopEvent);
        break;
    }
}

DWORD WINAPI WindowsService::ServiceWorkerThread(LPVOID lpParam)
{
    WindowsService::StartServer();
    while (WaitForSingleObject(g_ServiceStopEvent, 1000) != WAIT_OBJECT_0) 
    {
    }

    WindowsService::StopServer();
    return 0;
}

void WindowsService::StartServer() 
{
    LayoutManager::Load();
    Configuration::LoadFromFile("config.ini");
    bool useSSL = Configuration::GetBool("UseSSL", false);
    Web::ControllerRoutes routes;

    server = new Web::Server();
    server->UseSSL(useSSL);
    server->MapControllers(&routes);

    if (!server->Start()) 
    {
        SetEvent(g_ServiceStopEvent);
    }
}

void WindowsService::StopServer() 
{
    if (server) 
    {
        server->Stop();
    }
}