#include "endianness.h"

namespace util {

bool endianness::is_first = true;
bool endianness::is_little = true;

bool endianness::is_little_endian()
{
	if (endianness::is_first)
	{
		uint32_t test = 0xaabbccdd;
		uint8_t* lsbyte = (uint8_t*)&test;

		if (*lsbyte == 0xdd)
			endianness::is_little = true;
		else
			endianness::is_little = false;

		endianness::is_first = false;
	}

	return endianness::is_little;
}

}