#include "socks5_client.h"
#include "../crypto/encoding/base10.h"
#include "../util/endianness.h"

namespace net {


socks5_client::socks5_client()
{
	this->m_is_connected = false;
	this->m_is_proxy_set = false;
	this->m_socket = socket();
}

socks5_client::~socks5_client()
{
	this->disconnect();
}

bool socks5_client::is_connected()
{
	return this->m_is_connected;
}

bool socks5_client::is_proxy_set()
{
	return this->m_is_proxy_set;
}

int socks5_client::set_proxy(const char* proxy_ip, const char* proxy_port)
{
	int result = 0;
	
	result = m_socket.connect(proxy_ip, proxy_port);
	if (result != 0)
		return result;

	char client_greeting[4] = "\x05\x01\x00";
	m_socket.send(client_greeting, 3);

	char server_response[2] = { (char)0xff };
	m_socket.recv(server_response, 2);

	if (server_response[1] != 0x00)
		return -1;

	this->m_is_proxy_set = true;
	return 0;
}

int socks5_client::connect(const char* domain, const char* port)
{
	if (!this->m_is_proxy_set)
		return -1;

	uint8_t domain_length = (uint8_t)strlen(domain);
	if (domain_length > 121)
		return -1;

	uint16_t l_port = crypto::base10<uint16_t>::decode(port);
	if (util::endianness::is_little_endian())
		l_port = util::endianness::swap_endianness<uint16_t>(l_port);

	char client_connection_request[128] = "\x05\x01\x00\x03";
	client_connection_request[4] = domain_length;
	memcpy(&client_connection_request[5], domain, domain_length);
	memcpy(&client_connection_request[5 + domain_length], &l_port, 2);

	m_socket.send(client_connection_request, 7 + domain_length);

	char server_response[128] = { (char)0xff };
	m_socket.recv(server_response, 128);

	if (server_response[1] != 0x00)
		return (int)server_response[1];

	this->m_is_connected = true;
	return 0;
}

int socks5_client::send(char* in_buf, size_t size)
{
	if (!this->m_is_connected)
		return -1;

	int result = 0;
	result = m_socket.send(in_buf, size);
	return result;
}

int socks5_client::recv(char* out_buf, size_t size)
{
	if (!this->m_is_connected)
		return -1;

	int result = 0;
	result = m_socket.recv(out_buf, size);
	return result;
}

int socks5_client::disconnect()
{
	m_socket.shutdown();
	this->m_is_connected = false;
	this->m_is_proxy_set = false;

	return 0;
}


}