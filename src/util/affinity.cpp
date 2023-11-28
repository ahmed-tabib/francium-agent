#include "affinity.h"

namespace util
{

void affinity::set_thread_affinity(uint32_t cpuid, std::thread::native_handle_type thread)
{
	SetThreadAffinityMask(thread, 1ULL << cpuid);
}

void affinity::set_current_thread_affinity(uint32_t cpuid)
{
	affinity::set_thread_affinity(cpuid, GetCurrentThread());
}

#ifdef _DEBUG

DWORD affinity::get_thread_affinity_mask(std::thread::native_handle_type thread)
{
    DWORD mask = 1;
    DWORD old = 0;

    while (mask)
    {
        old = SetThreadAffinityMask(thread, mask);
        if (old)
        {
            SetThreadAffinityMask(thread, old);
            return old;
        }
        else
        {
            return 0;
        }
        mask <<= 1;
    }

    return 0;
}

DWORD affinity::get_current_thread_affinity_mask()
{
    return affinity::get_thread_affinity_mask(GetCurrentThread());
}

#endif

}