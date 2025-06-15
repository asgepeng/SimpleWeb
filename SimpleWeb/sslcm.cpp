#include "sslcm.h"

Web::SslContextManager::SslContextManager(SSL_CTX* context)
{
    defaultContext = context;
}

Web::SslContextManager::~SslContextManager()
{
	if (defaultContext != nullptr)
	{
		SSL_CTX_free(defaultContext);
	}
}

bool Web::SslContextManager::Initialize(std::string& certPath, std::string& keyPath)
{
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    defaultContext = SSL_CTX_new(TLS_server_method());
    if (!defaultContext)
    {
        ERR_print_errors_fp(stderr);
        return false;
    }

    if (SSL_CTX_use_certificate_file(defaultContext, certPath.c_str(), SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(defaultContext, keyPath.c_str(), SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        return false;
    }

    if (!SSL_CTX_check_private_key(defaultContext))
    {
        std::cerr << "Private key does not match the public certificate\n";
        return false;
    }
    return true;
}

void Web::SslContextManager::Cleanup()
{
    if (defaultContext != nullptr)
    {
        SSL_CTX_free(defaultContext);
    }
    for (auto& kv : sniContexts) {
        SSL_CTX_free(kv.second);
    }
    sniContexts.clear();
}

SSL_CTX* Web::SslContextManager::GetDefaultContext()
{
	return defaultContext;
}
