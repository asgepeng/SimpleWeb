#include "sslcm.h"
#include <openssl/err.h>
#include <iostream>

bool Web::SSLContextManager::Initialize(const std::string& certPath, const std::string& keyPath)
{
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    sslCtx = SSL_CTX_new(TLS_server_method());
    if (!sslCtx) 
    {
        ERR_print_errors_fp(stderr);
        return false;
    }

    if (SSL_CTX_use_certificate_file(sslCtx, certPath.c_str(), SSL_FILETYPE_PEM) <= 0) 
    {
        ERR_print_errors_fp(stderr);
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(sslCtx, keyPath.c_str(), SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        return false;
    }

    if (!SSL_CTX_check_private_key(sslCtx))
    {
        std::cerr << "Private key does not match the public certificate\n";
        return false;
    }

    return true;
}

void Web::SSLContextManager::Cleanup()
{
    if (sslCtx) 
    {
        SSL_CTX_free(sslCtx);
        sslCtx = nullptr;
    }
}
