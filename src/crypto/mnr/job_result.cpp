#include "job_result.h"

namespace mnr
{

job_result::job_result(uint8_t* h, uint32_t nonce, const job& j)
{
	for (int i = 0; i < RANDOMX_HASH_SIZE / sizeof(uint64_t); i++)
		((uint64_t*)hash)[i] = ((uint64_t*)h)[i];

	this->nonce = nonce;

	this->j = new job(j);
}

job_result::job_result(const job_result& other)
{
	for (int i = 0; i < RANDOMX_HASH_SIZE / sizeof(uint64_t); i++)
		((uint64_t*)hash)[i] = ((uint64_t*)other.hash)[i];

	this->nonce = other.nonce;

	this->j = new job(*other.j);
}

job_result::~job_result()
{
	if (this->j != nullptr)
		delete this->j;
}

}