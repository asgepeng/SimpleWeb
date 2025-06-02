#pragma once
#include <Windows.h>
#include <vector>

namespace Web
{
    class WorkerThreadPool {
    public:
        void Start(int threadCount, HANDLE hIOCP, LPTHREAD_START_ROUTINE threadFunc);
        void Stop();
    private:
        std::vector<HANDLE> threads;
        HANDLE iocpHandle = NULL;
    };
}

