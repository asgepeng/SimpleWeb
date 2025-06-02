#include "wthreadpool.h"

void Web::WorkerThreadPool::Start(int threadCount, HANDLE hIOCP, LPTHREAD_START_ROUTINE threadFunc)
{
    for (int i = 0; i < threadCount; ++i)
    {
        HANDLE hThread = CreateThread(nullptr, 0, threadFunc, hIOCP, 0, nullptr);
        if (hThread)
        {
            threads.push_back(hThread);
        }
    }
}
void Web::WorkerThreadPool::Stop()
{
    for (HANDLE hThread : threads) {
        PostQueuedCompletionStatus(iocpHandle, 0, 0, nullptr);
    }
    for (HANDLE hThread : threads) {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }
    threads.clear();
}
