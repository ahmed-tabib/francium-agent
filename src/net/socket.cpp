#include "socket.h"

namespace net {


bool socket::is_winsock_initialised = false;
int socket::instance_count = 0;
std::mutex socket::static_member_mutex;

socket::socket()
{
	m_socket = INVALID_SOCKET;

	std::lock_guard<std::mutex> mutex_guard(static_member_mutex);

	if (!is_winsock_initialised)
	{
		if (init_winsock() == 0)
			is_winsock_initialised = true;
		else
			return;
	}

	instance_count++;
}

socket::socket(const socket& other)
{
	if (!other.m_do_cleanup)
	{
		this->m_socket = other.m_socket;
		this->m_is_connect_socket = other.m_is_connect_socket;
		this->m_is_listen_socket = other.m_is_listen_socket;
		this->m_do_cleanup = true;
	}
	else
	{
		this->m_socket = INVALID_SOCKET;
	}
}

socket::~socket()
{
	if (m_do_cleanup)
	{
		this->shutdown();

		std::lock_guard<std::mutex> mutex_guard(static_member_mutex);

		instance_count--;

		if (instance_count == 0 && is_winsock_initialised)
		{
			WSACleanup();
			is_winsock_initialised = false;
		}
	}
}

bool socket::is_server()
{
	return m_is_listen_socket;
}

bool socket::is_client()
{
	return m_is_connect_socket;
}

int socket::init_winsock()
{
	WSADATA wsa_data;
	return WSAStartup(MAKEWORD(2, 2), &wsa_data);
}

int socket::connect(const char* ip, const char* port)
{
	int result = 0;

	if (m_is_connect_socket || m_is_listen_socket)
		return -1;

	addrinfo* addrinfo_result = nullptr;

	addrinfo addrinfo_hints;
	ZeroMemory(&addrinfo_hints, sizeof(addrinfo));
	addrinfo_hints.ai_family = AF_INET;
	addrinfo_hints.ai_socktype = SOCK_STREAM;
	addrinfo_hints.ai_protocol = IPPROTO_TCP;

	result = getaddrinfo(ip, port, &addrinfo_hints, &addrinfo_result);
	if (result != 0)
		return result;

	m_socket = ::socket(addrinfo_result->ai_family, addrinfo_result->ai_socktype, addrinfo_result->ai_protocol);
	if (m_socket == INVALID_SOCKET)
	{
		freeaddrinfo(addrinfo_result);
		return -1;
	}

	result = ::connect(m_socket, addrinfo_result->ai_addr, (int)addrinfo_result->ai_addrlen);
	if (result != 0)
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}
	else
	{
		m_is_connect_socket = true;
	}
	
	freeaddrinfo(addrinfo_result);
	return result;
}

int socket::send(char* in_buf, size_t size)
{
	int result = ::send(m_socket, in_buf, (int)size, NULL);
	return result;
}

int socket::recv(char* out_buf, size_t size)
{
	int result = ::recv(m_socket, out_buf, (int)size, NULL);
	return result;
}

int socket::bind(const char* port)
{
	int result = 0;

	if (m_is_connect_socket || m_is_listen_socket)
		return -1;

	addrinfo* addrinfo_result = nullptr;

	addrinfo addrinfo_hints;
	ZeroMemory(&addrinfo_hints, sizeof(addrinfo));
	addrinfo_hints.ai_family = AF_INET;
	addrinfo_hints.ai_socktype = SOCK_STREAM;
	addrinfo_hints.ai_protocol = IPPROTO_TCP;
	addrinfo_hints.ai_flags = AI_PASSIVE;

	result = getaddrinfo(NULL, port, &addrinfo_hints, &addrinfo_result);
	if (result != 0)
		return result;

	m_socket = ::socket(addrinfo_result->ai_family, addrinfo_result->ai_socktype, addrinfo_result->ai_protocol);
	if (m_socket == INVALID_SOCKET)
	{
		freeaddrinfo(addrinfo_result);
		return -1;
	}

	result = ::bind(m_socket, addrinfo_result->ai_addr, (int)addrinfo_result->ai_addrlen);
	if (result != 0)
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}
	else
	{
		m_is_listen_socket = true;
	}

	freeaddrinfo(addrinfo_result);
	return result;
}

int socket::listen(int max_queue)
{
	int result = SOCKET_ERROR;

	if (m_is_listen_socket)
	{
		result = ::listen(m_socket, max_queue);
	}

	return result;
}

socket socket::accept()
{
	socket r_socket = socket();
	r_socket.m_do_cleanup = false;

	r_socket.m_socket = ::accept(this->m_socket, NULL, NULL);

	if (r_socket.m_socket != INVALID_SOCKET)
		r_socket.m_is_connect_socket = true;

	return r_socket;
}

int socket::shutdown()
{
	if (m_socket != INVALID_SOCKET)
		closesocket(m_socket);

	m_socket = INVALID_SOCKET;
	this->m_is_connect_socket = false;
	this->m_is_listen_socket = false;

	return 0;
}


}