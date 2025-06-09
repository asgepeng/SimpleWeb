#pragma once
#include "server.h"

#include <windows.h>

class WindowsService {
public:
    WindowsService(const wchar_t* serviceName);
    ~WindowsService();

    bool Run();

private:
    static VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
    static VOID WINAPI ServiceCtrlHandler(DWORD ctrlCode);
    static DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

    static void StartServer();
    static void StopServer();
private:
    static inline HANDLE g_ServerThread = nullptr;
    static inline SERVICE_STATUS        g_ServiceStatus{};
    static inline SERVICE_STATUS_HANDLE g_StatusHandle = nullptr;
    static inline HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;
    static inline Web::Server* g_Server = nullptr;
    static inline const wchar_t* g_ServiceName = nullptr;
};