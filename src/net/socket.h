#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include <mutex>

namespace net {


class socket {
private:
	static std::mutex static_member_mutex;
	static bool is_winsock_initialised;
	static int instance_count;

	SOCKET m_socket;

	bool m_is_connect_socket = false;
	bool m_is_listen_socket = false;

	bool m_do_cleanup = true;

	int init_winsock();

public:
	socket();
	socket(const socket& other);
	~socket();

	bool is_server();
	bool is_client();

	int connect(const char* ip, const char* port);

	int send(char* in_buf, size_t size);
	int recv(char* out_buf, size_t size);

	int bind(const char* port);
	int listen(int max_queue = SOMAXCONN);
	socket accept();
	int shutdown();
};


}