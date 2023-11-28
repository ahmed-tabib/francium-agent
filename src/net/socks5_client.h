#pragma once

#include "socket.h"

namespace net {


class socks5_client {
private:

	bool m_is_proxy_set;
	bool m_is_connected;

	socket m_socket;

public:
	socks5_client();
	~socks5_client();

	bool is_proxy_set();
	bool is_connected();

	int set_proxy(const char* proxy_ip, const char* proxy_port);
	int connect(const char* domain, const char* port);
	int send(char* in_buf, size_t size);
	int recv(char* out_buf, size_t size);
	int disconnect();

};


}