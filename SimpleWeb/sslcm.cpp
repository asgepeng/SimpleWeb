#include "sslcm.h"

Web::SslManager::SslManager(SSL_CTX* context)
{
    sslContext = context;
}

Web::SslManager::~SslManager()
{
	if (sslContext != nullptr)
	{
		SSL_CTX_free(sslContext);
	}
}

bool Web::SslManager::Initialize(std::string& certPath, std::string& keyPath)
{
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    sslContext = SSL_CTX_new(TLS_server_method());
    if (!sslContext)
    {
        ERR_print_errors_fp(stderr);
        return false;
    }

    if (SSL_CTX_use_certificate_file(sslContext, certPath.c_str(), SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(sslContext, keyPath.c_str(), SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        return false;
    }

    if (!SSL_CTX_check_private_key(sslContext))
    {
        std::cerr << "Private key does not match the public certificate\n";
        return false;
    }
    return true;
}

void Web::SslManager::Cleanup()
{
    if (sslContext != nullptr)
    {
        SSL_CTX_free(sslContext);
    }
}

SSL_CTX* Web::SslManager::GetContext()
{
	return sslContext;
}
