#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <windows.h>

#include "net/socket.h"
#include "net/socks5_client.h"
#include "net/c2/fields.h"
#include "net/c2/messages.h"
#include "util/endianness.h"
#include "util/sysinfo.h"
#include "crypto/mnr/job.h"
#include "crypto/mnr/job_exec.h"
#include "crypto/mnr/controller.h"
#include "crypto/encoding/base10.h"
#include "ext/randomx/randomx.h"

#include <chrono>

int main()
{
	uint8_t b[4] = { 0x74, 0x74, 0x74, 0x74 };
	uint8_t s[32] = { 0 };

	// in memory: 0x00000074 -> stays the same
	//            0xffffff00 -> gets swapped
	mnr::job j(1, b, 4, 0x00ffffff, 0, s, 0x0000000000000001);

	j.init_nonce(0);

	for (size_t i = 0; i < 300; i++)
	{
		j.incr_nonce();

		if (i == 254)
			std::cout << "hi";
	}

	return 0;
}