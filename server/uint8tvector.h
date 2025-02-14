#ifndef UINT_8T_VECTOR
#define UINT_8T_VECTOR
#define INITIAL_CAPACITY 4
#include <stdlib.h>
#include <stdbool.h>

typedef struct
{
    size_t size;
    size_t capacity;
    u_int8_t *array;
} uint8TVector;

void freeVector(uint8TVector *vec);

void initVector(uint8TVector *vector);

bool resizeVector(uint8TVector *vec, size_t new_size, uint8_t value);

void appendToVector(uint8TVector *vector, const uint8_t *value, size_t len);

void eraseFromVector(uint8TVector *vector, size_t index);

#endif