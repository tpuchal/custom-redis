#ifndef POLL_FD_VECTOR_HEADER
#define POLL_FD_VECTOR_HEADER
#define INITIAL_CAPACITY 4


#include<stdlib.h>
#include<stdbool.h>

typedef struct pollfd pollfd;

typedef struct
{
    size_t size;
    size_t capacity;
    pollfd *array;
} pollFdVector;

void freepollFdVector(pollFdVector *vector);

void initPollFdVector(pollFdVector *vector);

bool resizePollFdVector(pollFdVector *vec, size_t new_size, int value);

void clearPollFdVector(pollFdVector *vector);

void pollVectorPushBack(pollFdVector *vector, pollfd value);

#endif