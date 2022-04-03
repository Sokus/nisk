/* date = November 26th 2021 12:29 pm */

#ifndef BASE_H
#define BASE_H

// common includes
#include <string.h>  // memcpy. memset
#include <stdbool.h> // bool
#include <stdint.h>  // int8_t, uint32_t
#include <stddef.h>  // size_t

// keywords
#define internal static
#define global static
#define local_persist static

// common macros
#define STATEMENT(statement) do { statement } while (0)
#define ASSERT_BREAK() (*(int *)0 = 0)
#if 1
#define ASSERT(expression) STATEMENT( if(!(expression)) { ASSERT_BREAK(); } )
#else
#define ASSERT(expression)
#endif

#define KILOBYTES(value) ((value)*1024LL)
#define MEGABYTES(value) (KILOBYTES(value)*1024LL)
#define GIGABYTES(value) (MEGABYTES(value)*1024LL)
#define TERABYTES(value) (GIGABYTES(value)*1024LL)

#define INVALID_CODE_PATH ASSERT("" == 0)

#define ARRAY_SIZE(array) ( sizeof(array)/sizeof((array)[0]) )

#define MIN(a, b) (((a)<(b)) ? (a) : (b))
#define MAX(a, b) (((a)<(b)) ? (b) : (a))
#define CLAMP(a, x, b) (((x)<(a))?(a):\
((x)>(b))?(b):(x))
#define CLAMP_TOP(a, b) MIN(a, b)
#define CLAMP_BOT(a, b) MAX(a, b)

#define ABS(a) ((a) >= 0) ? (a) : -(a)

#define SWAP(a, b, type) STATEMENT(type swap=a; a=b; b=swap;)

#define SIGN(x) (((x) > 0) - ((x) < 0))

#define MEMORY_COPY(destination, source, size) memcpy(destination, source, size)
#define MEMORY_SET(destination, value, size) memset(destination, value, size)

//~NOTE(sokus): memory arenas

typedef struct MemoryArena
{
    uint8_t *base;
    size_t size;
    size_t used;
} MemoryArena;

internal void InitializeArena(MemoryArena *arena, uint8_t *base, size_t size)
{
    arena->base = base;
    arena->size = size;
    arena->used = 0;
}

#define PUSH_STRUCT(arena, type) (type *)MemoryArenaPushSize(arena, sizeof(type))
#define PUSH_ARRAY(arena, type, count) (type *)MemoryArenaPushSize(arena, (count)*sizeof(type)) 

void *MemoryArenaPushSize(MemoryArena *arena, size_t size)
{
    bool can_fit = ((arena->used + size) <= arena->size);
    void *result = 0;
    ASSERT(can_fit);
    if(can_fit)
    {
        result = arena->base + arena->used;
        arena->used += size;
    }
    return result;
}

void *MemoryArenaPopSize(MemoryArena *arena, size_t size)
{
    ASSERT(size > 0);
    ASSERT(arena->used >= size);
    size_t new_used = arena->used - size;
    arena->used = new_used;
    void *result = arena->base + new_used;
    return result;
}

//~NOTE(sokus): string

void ConcatenateStrings(char *str_a, size_t str_a_size,
                        char *str_b, size_t str_b_size,
                        char *dest, size_t dest_size)
{
    size_t dest_size_clamped = MAX(0, dest_size);
    size_t str_a_size_clamped = MIN(str_a_size, dest_size_clamped);
    size_t size_remaining = dest_size_clamped - str_a_size_clamped;
    size_t str_b_size_clamped = MIN(str_b_size, size_remaining);
    
    MEMORY_COPY(dest, str_a, str_a_size_clamped);
    MEMORY_COPY(dest+str_a_size_clamped, str_b, str_b_size_clamped);
    
    *(dest+str_a_size_clamped+str_b_size_clamped) = 0;
}

unsigned int StringLength(char *str)
{
    unsigned int result = 0;
    while(*str++)
    {
        ++result;
    }
    return result;
}

bool StringCompare(char *str_a, unsigned int str_a_size,
                   char *str_b, unsigned int str_b_size)
{
    if(str_a == 0 || str_b == 0)
        return false;
    
    if(str_a_size != str_b_size)
        return false;
    
    for(unsigned int idx = 0; idx < str_a_size; ++idx)
    {
        if(*(str_a + idx) != *(str_b + idx))
            return false;
    }
    
    return true;
}

#endif // BASE_H
