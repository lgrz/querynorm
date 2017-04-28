#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>

#ifndef DEBUG
#define DLOG(...)
#else
#define DLOG(...) do {\
    fprintf(stderr, __VA_ARGS__); \
} while (0)
#endif /* DEBUG */

void *
bmalloc(size_t size);

void *
brealloc(void *old_mem, size_t new_size);

#endif /* UTIL_H */
