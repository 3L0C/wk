#ifndef WK_COMMON_MEMORY_H_
#define WK_COMMON_MEMORY_H_

#include <stddef.h>

#define ALLOCATE(type, count) (type*)reallocate(NULL, 0, sizeof(type) * (count))

void* reallocate(void* pointer, size_t oldSize, size_t newSize);

#endif /* WK_COMMON_MEMORY_H_ */
