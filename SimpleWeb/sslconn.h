#pragma once
#include "conn.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

namespace Web
{
	class SecureConnection : public Connection
	{
	public:
		SecureConnection(SOCKET connectedSocket, SSL* ssl);
		~SecureConnection();

		bool HandshakeFinished();
		bool WriteBIO();
		bool ReadBIO();
	private:
		SSL* ssl;
		BIO* rbio;
		BIO* wbio;
	};
}
