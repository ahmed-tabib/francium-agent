#pragma once

#include "job.h"
#include "../../ext/randomx/randomx.h"

namespace mnr
{

struct job_result
{
	uint8_t hash[RANDOMX_HASH_SIZE];
	uint32_t nonce;
	job* j = nullptr;

	job_result(uint8_t* h, uint32_t nonce, const job& j);
	job_result(const job_result& other);
	~job_result();
};

}