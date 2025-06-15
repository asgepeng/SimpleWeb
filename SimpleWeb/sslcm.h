#pragma once
#include "appconfig.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <string>
#include <unordered_map>

namespace Web
{
	class SslContextManager
	{
	public:
		static int SniCallback(SSL* ssl, int* ad, void* arg) {
			auto* manager = static_cast<SslContextManager*>(arg);
			const char* servername = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
			if (!servername) return SSL_TLSEXT_ERR_NOACK;

			SSL_CTX* selected = manager->GetContextForHost(servername);
			if (selected) {
				SSL_set_SSL_CTX(ssl, selected);
				return SSL_TLSEXT_ERR_OK;
			}

			return SSL_TLSEXT_ERR_NOACK;
		}
		SslContextManager(SSL_CTX* context);
		~SslContextManager();

		bool Initialize(std::string& certPath, std::string& keyPath);
		void Cleanup();
		SSL_CTX* GetDefaultContext();
		SSL_CTX* GetContextForHost(const std::string& hostname)
		{
			return sniContexts[hostname];
		}
		void AddSniContext(const std::string& hostname, SSL_CTX* ctx) {
			sniContexts[hostname] = ctx;
		}
	private:
		SSL_CTX* defaultContext;
		std::unordered_map<std::string, SSL_CTX*> sniContexts;
	};
}

