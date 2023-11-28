#include "job.h"
#include "../../util/endianness.h"

#include <iostream>

namespace mnr{

job::job(uint32_t id, uint8_t* blob, size_t size, uint32_t nonce_mask, uint32_t nonce_offset, uint8_t* seed_hash, uint64_t target)
{
	m_id = id;
	m_blob_size = size;

	m_blob = new uint8_t[m_blob_size];
	for (size_t i = 0; i < m_blob_size; i++)
		m_blob[i] = blob[i];

	m_nonce = (uint32_t*)(m_blob + nonce_offset);
	m_nonce_mask = nonce_mask;

	for (size_t i = 0; i < 32; i++)
		this->m_seed_hash[i] = seed_hash[i];

	m_target = target;


	//if the nonce is little endian, At least the LSB of the nonce mask will be 0 and the MSB will be 1,
	//the opposite if it is big endian. we don't swap if they are both 1 since the whole nonce is available
	//and it would be inefficient to swap the endianness of a nonce we're free to modify it as we like
	//Therefore we only swap if the nonce endianness does not match the system endianness, Hence:
	bool MSB = (m_nonce_mask & ((uint32_t)1 << 31)) != 0;
	bool LSB = (m_nonce_mask & (uint32_t)1) != 0;
	m_endian_swap_condition = !(MSB && LSB) && ((MSB && !LSB) != util::endianness::is_little_endian());

	//we also need to undo the endian swap that was done on the nonce mask
	if (!m_endian_swap_condition && util::endianness::is_little_endian())
		m_nonce_mask = util::endianness::swap_endianness<uint32_t>(m_nonce_mask);
}

job::job(const job& other)
{
	m_id = other.m_id;
	m_blob_size = other.m_blob_size;

	m_blob = new uint8_t[m_blob_size];
	for (size_t i = 0; i < m_blob_size; i++)
		m_blob[i] = other.m_blob[i];

	m_nonce = (uint32_t*)(m_blob + (uint32_t)((uint8_t*)other.m_nonce - other.m_blob));
	m_nonce_mask = other.m_nonce_mask;
	m_swapped_endian_nonce = other.m_swapped_endian_nonce;
	m_is_nonce_init = false;

	for (size_t i = 0; i < 32; i++)
		this->m_seed_hash[i] = other.m_seed_hash[i];

	m_target = other.m_target;

	if (other.m_is_nonce_init)
		this->init_nonce(other.m_nonce_seed);

	m_endian_swap_condition = other.m_endian_swap_condition;
}

job::~job()
{
	delete[] m_blob;
}

void job::init_nonce(uint32_t seed)
{
	m_nonce_seed = seed;

	uint32_t nonce = (m_endian_swap_condition) ? util::endianness::swap_endianness<uint32_t>(*m_nonce) : *m_nonce;
	nonce &= ~m_nonce_mask;
	nonce |= seed & m_nonce_mask;
	*m_nonce = (m_endian_swap_condition) ? util::endianness::swap_endianness<uint32_t>(nonce) : nonce;
	m_swapped_endian_nonce = nonce;

	m_is_nonce_init = true;
}

void job::incr_nonce()
{
	if (m_endian_swap_condition)
	{
		m_swapped_endian_nonce = (m_swapped_endian_nonce & ~m_nonce_mask) | ((m_swapped_endian_nonce & m_nonce_mask) + 1);
		*m_nonce = util::endianness::swap_endianness<uint32_t>(m_swapped_endian_nonce);
	}
	else
	{
		*m_nonce = (*m_nonce & ~m_nonce_mask) | ((*m_nonce & m_nonce_mask) + 1);
	}
}

uint32_t job::get_nonce()
{
	return *m_nonce;
}

uint32_t job::get_id()
{
	return m_id;
}

uint32_t job::get_iteration_count()
{
	return this->m_nonce_mask;
}

size_t job::get_size()
{
	return m_blob_size;
}

uint8_t* job::get_blob()
{
	return m_blob;
}

uint8_t* job::get_seed_hash()
{
	return m_seed_hash;
}

uint64_t job::get_target()
{
	return m_target;
}

job_pair job::split_job()
{
	return job_pair(*this);
}

job_pair::job_pair(const job& j) : job1(j), job2(j)
{
	if (j.m_endian_swap_condition)
	{
		uint32_t nonce = util::endianness::swap_endianness<uint32_t>(*j.m_nonce);
		uint32_t old_mask = j.m_nonce_mask;
		uint32_t new_mask = old_mask >> 1;
		
		job1.m_nonce_mask = new_mask;
		job2.m_nonce_mask = new_mask;

		nonce &= ~old_mask;

		*job1.m_nonce = util::endianness::swap_endianness<uint32_t>(nonce);

		nonce |= (old_mask ^ new_mask);

		*job2.m_nonce = util::endianness::swap_endianness<uint32_t>(nonce);
	}
	else
	{
		job1.m_nonce_mask >>= 1;
		job2.m_nonce_mask >>= 1;
		*job1.m_nonce = (*j.m_nonce & ~j.m_nonce_mask);
		*job2.m_nonce = (*j.m_nonce & ~j.m_nonce_mask) | (j.m_nonce_mask ^ job2.m_nonce_mask);
	}
}

job_pair::job_pair(const job_pair& other): job1(other.job1), job2(other.job2)
{
}

}