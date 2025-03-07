#ifndef UTILS_COMMON_H_
#define UTILS_COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory>
#include <functional>

#include <errno.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#include <string>
#include <string.h>
#include <list>
#include <vector>
#include <queue>
#include <map>
#include <set>
#include <algorithm>


#ifndef NDEBUG
#include <assert.h>
#endif

#define MSG_NOSIGNAL_ 0
#define SIGHUP       1
#define SIGQUIT      3
#define SIGKILL      9

#include <pthread.h>
#include <limits.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <ifaddrs.h>
#include <dirent.h>

#define ERROR_SUCCESS            0
#define ERROR_ALREADY_EXISTS     EEXIST
#define ERROR_NOT_READY          ENOENT
#define ERROR_ACCESS_DENIED      EPERM
#define ERROR_NO_DATA            ENOENT
#define ERROR_TIMEOUT            ETIMEDOUT
#define ERROR_CONNECTION_REFUSED ECONNREFUSED
#define ERROR_INVALID_PARAMETER  ERANGE
#define ERROR_NOT_SUPPORTED      ENODATA
#define ERROR_INTERNAL_ERROR     EFAULT
#define ERROR_IO_DEVICE          EIO

#define fseek64     fseeko
#define ftell64     ftello
#define closesocket close

#ifdef __x86_64__
#define FMT_I64 "%ld"
#define FMT_I64_EX(fmt) "%" #fmt "ld"
#define FMT_U64 "%lu"
#define FMT_X64 "%lX"
#define FMT_SIZE "%lu"
#else
#define FMT_I64 "%lld"
#define FMT_I64_EX(fmt) "%" #fmt "lld"
#define FMT_U64 "%llu"
#define FMT_X64 "%llX"
#define FMT_SIZE "%u"
#endif

namespace utils {

	using namespace std::placeholders; // for std::bind

	// It is not allowed to copy ctor and assign opt
#undef UTILS_DISALLOW_EVIL_CONSTRUCTORS
#define UTILS_DISALLOW_EVIL_CONSTRUCTORS(TypeName)    \
    TypeName(const TypeName&);                         \
    void operator=(const TypeName&)

	// Delete the object safely
#define SAFE_DELETE(p)        \
    if (NULL != p) {          \
        delete p;             \
        p = NULL;             \
	    }

	// Delete the object array safely
#define SAFE_DELETE_ARRAY(p)  \
    if (NULL != p) {          \
        delete []p;           \
        p = NULL;             \
	    }

#ifndef MIN
#define MIN(a,b) ((a)<(b)) ? (a) : (b)
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)) ? (a) : (b)
#endif

#define CHECK_ERROR_RET(func, ecode, ret) \
	if( func )\
		{\
	utils::set_error_code(ecode); \
	return ret; \
		}

#define DISALLOW_COPY_AND_ASSIGN(cls) private:\
	cls( const cls & );\
	cls & operator =( const cls & )
}


#define VALIDATE_ERROR_RETURN(expr, ecode, ret) \
	if( expr )\
						{\
	utils::set_error_code(ecode); \
	return ret; \
	}
	
extern "C"
{
	void * __wrap_memcpy(void *dest, const void *src, size_t n);
}

//#Define DEBUG_NEW_ENABLE
//#Include "debug_new.h"

#endif 
