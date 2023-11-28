#pragma once

#include <memory>
#include <vector>
#include <string>
#include "../../util/endianness.h"

namespace net
{


template <typename value_type, bool swap_little_endian = true>
class integer_field
{
public:

	value_type& value() { return m_value; }
	size_t length() { return sizeof(value_type); }

	template <typename read_iter>
	void read(read_iter& iter, size_t len)
	{
		uint8_t* output_iter = (uint8_t*)&m_value;
		read_iter end = (sizeof(value_type) < len) ? iter + sizeof(value_type) : iter + len; 

		while (iter != end)
		{
			*output_iter = *iter;
			++output_iter; ++iter;
		}

		if (util::endianness::is_little_endian() && swap_little_endian)
			m_value = util::endianness::swap_endianness<value_type>(m_value);
	}

	template <typename write_iter>
	void write(write_iter& iter, size_t len)
	{
		value_type tmp = m_value;

		if (util::endianness::is_little_endian() && swap_little_endian)
			tmp = util::endianness::swap_endianness<value_type>(tmp);

		uint8_t* input_iter = (uint8_t*)&tmp;
		write_iter end = (sizeof(value_type) < len) ? iter + sizeof(value_type) : iter + len;

		while (iter != end)
		{
			*iter = *input_iter;
			++input_iter; ++iter;
		}
	}

private:
	value_type m_value = 0;
};

template <bool has_length_field, typename length_field_type = uint32_t>
class buffer_field
{
public:

	std::vector<uint8_t>& value() { return m_value; }
	size_t length() { return m_value.size() + ((has_length_field) ? len_field.length() : 0); }
	void set_length(length_field_type new_length) { len_field.value() = new_length; m_value.resize(new_length); }

	template <typename read_iter>
	void read(read_iter& iter, size_t len)
	{
		if (has_length_field)
			len_field.read(iter, len);

		m_value.clear();
		m_value.shrink_to_fit();

		read_iter end = (len_field.value() < len) ? iter + len_field.value() : iter + len;
		m_value.resize(end - iter, 0x00);

		std::vector<uint8_t>::iterator output_iter = m_value.begin();

		while (iter != end)
		{
			*output_iter = *iter;
			output_iter++; iter++;
		}
	}

	template <typename write_iter>
	void write(write_iter& iter, size_t len)
	{
		len_field.value() = m_value.size();

		if (has_length_field)
			len_field.write(iter, len);

		write_iter end = (len_field.value() < len) ? iter + len_field.value() : iter + len;

		std::vector<uint8_t>::iterator input_iter = m_value.begin();

		while (iter != end)
		{
			*iter = *input_iter;
			input_iter++; iter++;
		}
	}

private:
	std::vector<uint8_t> m_value;
	integer_field<length_field_type> len_field;
};

template <bool has_length_field, typename length_field_type = uint32_t>
class string_field
{
public:

	std::string& value() { return m_value; }
	size_t length() { return m_value.size() + ((has_length_field) ? len_field.length() : 0); }
	void set_length(length_field_type new_length) { len_field.value() = new_length; m_value.resize(new_length); }

	template <typename read_iter>
	void read(read_iter& iter, size_t len)
	{
		if (has_length_field)
			len_field.read(iter, len);

		m_value.clear();
		m_value.shrink_to_fit();

		read_iter end = (len_field.value() < len) ? iter + len_field.value() : iter + len;
		m_value.resize(end - iter, 0x00);

		std::string::iterator output_iter = m_value.begin();

		while (iter != end)
		{
			*output_iter = *iter;
			output_iter++; iter++;
		}
	}

	template <typename write_iter>
	void write(write_iter& iter, size_t len)
	{
		len_field.value() = m_value.size();

		if (has_length_field)
			len_field.write(iter, len);

		write_iter end = (len_field.value() < len) ? iter + len_field.value() : iter + len;

		std::string::iterator input_iter = m_value.begin();

		while (iter != end)
		{
			*iter = *input_iter;
			input_iter++; iter++;
		}
	}

private:
	std::string m_value;
	integer_field<length_field_type> len_field;
};

}