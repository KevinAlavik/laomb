#ifndef _LIBALLOC_H
#define _LIBALLOC_H

#define PREFIX(func)		k ## func

#include <stddef.h>
#include <stdint.h>

extern void    *PREFIX(malloc)(size_t);
extern void    *PREFIX(realloc)(void *, size_t);
extern void    *PREFIX(calloc)(size_t, size_t);
extern void     PREFIX(free)(void *);

#endif