#include <stdint.h>
#include <stddef.h>

#define FNV_OFFSET_32 2166136261U
#define FNV_PRIME_32 16777619U

#define FNV_OFFSET_64 14695981039346656037ULL
#define FNV_PRIME_64 1099511628211ULL

uint32_t fnv1a_32(const void *key, size_t len)
{
    uint32_t hash = FNV_OFFSET_32;
    const uint8_t *data = (const uint8_t *)key;

    for (size_t i = 0; i < len; i++)
    {
        hash ^= data[i];
        hash *= FNV_PRIME_32;
    }

    return hash;
}

uint64_t fnv1a_64(const void *key, size_t len)
{
    uint64_t hash = FNV_OFFSET_64;
    const uint8_t *data = (const uint8_t *)key;

    for (size_t i = 0; i < len; i++)
    {
        hash ^= data[i];
        hash *= FNV_PRIME_64;
    }

    return hash;
}
