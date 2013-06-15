
#include "PancakeDebug.h"

#ifdef PANCAKE_DEBUG

/* Directly allocate memory allocation information hash tables to prevent recursion in allocator */
#undef uthash_malloc
#undef uthash_free

#define uthash_malloc malloc
#define uthash_free(ptr, sz) free(ptr)

static PancakeAllocatedMemory *allocated = NULL;

PANCAKE_API void _PancakeAssert(Native result, Byte *condition, Byte *file, Int32 line) {
	if(!result) {
		printf("Assertion failed: %s in file %s on line %i\n", condition, file, line);

#ifdef HAVE_EXECINFO_H
		void *array[50];
		Native size, i;
		Byte **strings;

		printf("Backtrace:\n");

		size = backtrace(array, 50);
		strings = backtrace_symbols(array, size);

		if(strings == NULL) {
			exit(1);
		}

		for(i = 0; i < size; i++) {
			printf("#%li %s\n", size - i, strings[i]);
		}

		free(strings);
#endif

		exit(1);
	}
}

PANCAKE_API void *_PancakeAllocate(Native size, Byte *file, Int32 line) {
	void *ptr = malloc(size);
	PancakeAllocatedMemory *mem;

	_PancakeAssert(ptr != NULL, "Out of memory", file, line);

	mem = malloc(sizeof(PancakeAllocatedMemory));
	_PancakeAssert(mem != NULL, "Out of memory", file, line);

	mem->file = file;
	mem->line = line;
	mem->ptr = ptr;
	mem->size = size;

	HASH_ADD(hh, allocated, ptr, sizeof(void*), mem);

	return ptr;
}

PANCAKE_API void *_PancakeReallocate(void *ptr, Native size, Byte *file, Int32 line) {
	PancakeAllocatedMemory *mem;
	void *newPtr;

	if(ptr == NULL) {
		return _PancakeAllocate(size, file, line);
	}

	if(size == 0) {
		_PancakeFree(ptr, file, line);
		return NULL;
	}

	HASH_FIND(hh, allocated, &ptr, sizeof(void*), mem);
	_PancakeAssert(mem != NULL, "Trying to reallocate invalid pointer", file, line);

	newPtr = realloc(ptr, size);
	_PancakeAssert(newPtr != NULL, "Out of memory", file, line);

	mem->ptr = newPtr;
	mem->size = size;

	HASH_DEL(allocated, mem);
	HASH_ADD(hh, allocated, ptr, sizeof(void*), mem);

	return newPtr;
}

PANCAKE_API void _PancakeFree(void *ptr, Byte *file, Int32 line) {
	PancakeAllocatedMemory *mem;

	_PancakeAssert(ptr != NULL, "Trying to free NULL pointer", file, line);

	HASH_FIND(hh, allocated, &ptr, sizeof(void*), mem);
	_PancakeAssert(mem != NULL, "Trying to free invalid pointer", file, line);

	free(ptr);
	HASH_DEL(allocated, mem);
	free(mem);
}

PANCAKE_API Byte *_PancakeDuplicateString(Byte *string, Byte *file, Int32 line) {
	Byte *ptr;
	PancakeAllocatedMemory *mem;

	_PancakeAssert(string != NULL, "Trying to duplicate NULL pointer", file, line);

	ptr = strdup(string);
	_PancakeAssert(ptr != NULL, "Out of memory", file, line);

	mem = malloc(sizeof(PancakeAllocatedMemory));
	_PancakeAssert(mem != NULL, "Out of memory", file, line);

	mem->file = file;
	mem->line = line;
	mem->ptr = ptr;
	mem->size = strlen(string) + 1;

	HASH_ADD(hh, allocated, ptr, sizeof(void*), mem);

	return ptr;
}

PANCAKE_API Byte *_PancakeDuplicateStringLength(Byte *string, Int32 length, Byte *file, Int32 line) {
	Byte *ptr;
	PancakeAllocatedMemory *mem;

	_PancakeAssert(string != NULL, "Trying to duplicate NULL pointer", file, line);

	ptr = strndup(string, length);
	_PancakeAssert(ptr != NULL, "Out of memory", file, line);

	mem = malloc(sizeof(PancakeAllocatedMemory));
	_PancakeAssert(mem != NULL, "Out of memory", file, line);

	mem->file = file;
	mem->line = line;
	mem->ptr = ptr;
	mem->size = length + 1;

	HASH_ADD(hh, allocated, ptr, sizeof(void*), mem);

	return ptr;
}

PANCAKE_API void PancakeDumpHeap() {
	PancakeAllocatedMemory *mem;
	UNative total = 0;

	for(mem = allocated; mem != NULL; mem = mem->hh.next) {
		printf("[%#lx] %u bytes allocated in %s on line %i\n", (UNative) mem->ptr, mem->size, mem->file, mem->line);
		total += mem->size;
	}

	if(total) {
		printf("%lu bytes total allocated\n", total);
	}
}
#endif
