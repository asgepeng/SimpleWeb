#include "sslconn.h"

Web::SecureConnection::SecureConnection(SOCKET connectedSocket, SSL* ssl) : Connection(connectedSocket), ssl(ssl), rbio(nullptr), wbio(nullptr)
{

}

Web::SecureConnection::~SecureConnection()
{
	if (ssl != nullptr)
	{
		SSL_shutdown(ssl);
		SSL_free(ssl);
	}
}

bool Web::SecureConnection::HandshakeFinished()
{
	return SSL_is_init_finished(ssl);
}

bool Web::SecureConnection::WriteBIO()
{
	return false;
}

bool Web::SecureConnection::ReadBIO()
{
	return false;
}
