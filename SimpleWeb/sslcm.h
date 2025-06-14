#pragma once
#include "appconfig.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <string>

namespace Web
{
	class SslManager
	{
	public:
		SslManager(SSL_CTX* context);
		~SslManager();

		bool Initialize(std::string& certPath, std::string& keyPath);
		void Cleanup();
		SSL_CTX* GetContext();
	private:
		SSL_CTX* sslContext;
	};
}

