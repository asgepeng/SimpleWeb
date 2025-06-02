#pragma once
#include <string>
#include <openssl/ssl.h>

namespace Web
{
    class SSLContextManager 
    {
    public:
        SSLContextManager() : sslCtx(nullptr) {}
        ~SSLContextManager() { Cleanup(); }
        bool Initialize(const std::string& certPath, const std::string& keyPath);
        SSL_CTX* GetContext() const { return sslCtx;  }
        void Cleanup();
    private:
        SSL_CTX* sslCtx = nullptr;
    };
}
