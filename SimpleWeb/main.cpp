#include "service.h"
#include "layoutmanager.h"

int main()
{
    LayoutManager::Load();
    const wchar_t* serviceName = L"SimpleWeb";
    WindowsService service((LPWSTR)serviceName);
    return service.Run() ? 0 : 1;
}