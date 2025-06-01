#include "service.h"

int main()
{
    const wchar_t* serviceName = L"SimpleWeb";
    WindowsService service((LPWSTR)serviceName);
    return service.Run() ? 0 : 1;
}
