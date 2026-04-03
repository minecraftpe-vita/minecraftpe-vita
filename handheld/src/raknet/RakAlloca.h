#if defined(__FreeBSD__)
#include <stdlib.h>




#elif defined ( __APPLE__ ) || defined ( __APPLE_CC__ )
#include <malloc/malloc.h>
#include <alloca.h>
#elif defined(_WIN32)
#include <malloc.h>
#elif defined(__EPOC32__)
#include <stdlib.h>
#define alloca __builtin_alloca
#else
#include <malloc.h>
// Alloca needed on Ubuntu apparently
#include <alloca.h>
#endif
