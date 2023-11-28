#pragma once

#include <stdint.h>

namespace util {

class endianness {
private:
	static bool is_little;
	static bool is_first;

public:
	static bool is_little_endian();

	template <typename T>
	static T swap_endianness(T var)
	{
		T res = 0;
		for (int i = 0; i < sizeof(T); i++)
		{
			((char*)&res)[i] = ((char*)&var)[sizeof(T) - i - 1];
		}
		return res;
	}

};

}