#include "LocklessTypes.h"

#ifdef __EPOC32__
#include <e32atomics.h>
#endif

using namespace RakNet;

LocklessUint32_t::LocklessUint32_t()
{
	value=0;
}
LocklessUint32_t::LocklessUint32_t(uint32_t initial)
{
	value=initial;
}
uint32_t LocklessUint32_t::Increment(void)
{
#ifdef _WIN32
	return (uint32_t) InterlockedIncrement(&value);
#elif defined(__EPOC32__)
	return __e32_atomic_add_ord32 (&value, 1);
#elif defined(ANDROID) || defined(__S3E__)
	uint32_t v;
	mutex.Lock();
	++value;
	v=value;
	mutex.Unlock();
	return v;
#else
	return __sync_fetch_and_add (&value, (uint32_t) 1);
#endif
}
uint32_t LocklessUint32_t::Decrement(void)
{
#ifdef _WIN32
	return (uint32_t) InterlockedDecrement(&value);
#elif defined(ANDROID) || defined(__S3E__)
	uint32_t v;
	mutex.Lock();
	--value;
	v=value;
	mutex.Unlock();
	return v;
#elif defined(__EPOC32__)
	return __e32_atomic_add_ord32 (&value, (TUint32) -1);
#else
	return __sync_fetch_and_add (&value, (uint32_t) -1);
#endif
}
