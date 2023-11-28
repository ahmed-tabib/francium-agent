#pragma once

template<unsigned int SIZE, const char* KEY, unsigned int KEY_SIZE>
class o_string {
	char m_data[SIZE] = {0};

public:
	constexpr o_string(const char* in_buf)
	{
		for (unsigned int i = 0; i < SIZE; i++)
			m_data[i] = in_buf[i] ^ KEY[i % KEY_SIZE];
	}

	void get_string(char* out_buf) const
	{
		for (unsigned int i = 0; i < SIZE; i++)
			out_buf[i] = m_data[i] ^ KEY[i % KEY_SIZE];
	}

	constexpr char* get_data()
	{
		return m_data;
	}

	constexpr unsigned int get_size() const
	{
		return SIZE;
	}
};