#pragma once

#include <stdint.h>

namespace mnr
{

class job
{
private:
	friend struct job_pair;

	uint32_t m_id;

	uint8_t* m_blob;
	size_t m_blob_size;
	uint32_t* m_nonce;
	uint32_t m_swapped_endian_nonce;
	uint32_t m_nonce_mask;
	uint32_t m_nonce_seed;


	uint8_t m_seed_hash[32];

	uint64_t m_target;

	bool m_is_nonce_init = false;
	bool m_endian_swap_condition = false;
	
public:
	job(uint32_t id, uint8_t* blob, size_t blob_size, uint32_t nonce_mask, uint32_t nonce_offset, uint8_t* seed_hash, uint64_t target);
	job(const job& other);
	~job();

	void init_nonce(uint32_t seed);
	void incr_nonce();
	
	uint32_t get_id();
	size_t get_size();
	uint8_t* get_blob();
	uint32_t get_nonce();
	uint8_t* get_seed_hash();
	uint32_t get_iteration_count();
	uint64_t get_target();

	job_pair split_job();
};

struct job_pair {
	job_pair(const job& j);
	job_pair(const job_pair& other);
	job job1;
	job job2;
};

}