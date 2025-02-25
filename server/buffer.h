#ifndef UINT_8T_VECTOR
#define UINT_8T_VECTOR
#define INITIAL_CAPACITY 4
#define MAX_BUFFER_SIZE 10000
#include <stdlib.h>
#include <stdbool.h>

typedef struct
{
    uint8_t *data_begin;
    uint8_t *data_end;
    uint8_t *buffer_begin;
    uint8_t *buffer_end;
} Buffer;

bool consumeNewBuffer(Buffer *buffer, size_t data_size);

bool appendToNewBuffer(Buffer *buffer, const uint8_t *data, size_t data_size);

void initBuffer(Buffer* buffer);

void freeBuffer(Buffer *buffer);

#endif