
#ifndef _PANCAKE_H
#define _PANCAKE_H

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#define _GNU_SOURCE

/* System includes */
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <arpa/inet.h>

/* uthash library */
#include <uthash.h>
#include <utlist.h>

/* Let uthash allocate the tables via Pancake */
#undef uthash_malloc
#undef uthash_free

#define uthash_malloc PancakeAllocate
#define uthash_free(ptr, sz) PancakeFree(ptr)

#if defined(__GNUC__) && __GNUC__ >= 4
#	define PANCAKE_API __attribute__ ((visibility("default")))
#else
#	define PANCAKE_API
#endif

/* Type definitions */
#if defined(__x86_64__)
#	if SIZEOF_LONG == 4
typedef long long Native;
typedef unsigned long long UNative;
#	elif SIZEOF_LONG == 8
typedef long Native;
typedef unsigned long UNative;
#	else
#		error "Unknown size of long"
#	endif
#else
typedef int Native;
typedef unsigned int UNative;
#endif

typedef char Byte;
typedef char Int8;
typedef short Int16;
typedef int Int32;
typedef long long Int64;

typedef unsigned char UByte;
typedef unsigned char UInt8;
typedef unsigned short UInt16;
typedef unsigned int UInt32;
typedef unsigned long long UInt64;

typedef struct _String {
	UByte *value;
	UNative length;
} String;

typedef UByte (*PancakeModuleInitializeFunction)();
typedef void (*PancakeWorkerEntryFunction)();

typedef struct _PancakeWorker {
	String name;
	PancakeWorkerEntryFunction run;
	Int32 pid;
} PancakeWorker;

typedef struct _PancakeModule {
	UByte *name;
	PancakeModuleInitializeFunction init;
	PancakeModuleInitializeFunction configurationLoaded;
	PancakeModuleInitializeFunction shutdown;

	UByte intialized;
} PancakeModule;

#include "PancakeNetwork.h"

typedef struct _PancakeMainConfigurationStructure {
	/* Logging */
	FILE *systemLog;
	FILE *requestLog;
	FILE *errorLog;

	/* Workers */
	Int32 workers;
	Byte *user;
	Byte *group;
	Int32 concurrencyLimit;

	/* ServerArchitecture */
	PancakeServerArchitecture *serverArchitecture;
} PancakeMainConfigurationStructure;

extern PancakeWorker PancakeCurrentWorker;
extern PancakeModule *PancakeModules[];
extern PancakeMainConfigurationStructure PancakeMainConfiguration;

/* Pancake version constant */
#define PANCAKE_VERSION "2.0"
#define PANCAKE_COPYRIGHT "2012 - 2013 Yussuf Khalil"

#include "PancakeDebug.h"

#endif
