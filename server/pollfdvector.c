#include "pollfdvector.h"
#include <poll.h>
#include <stdbool.h>

void freepollFdVector(pollFdVector *vector)
{
    free(vector->array);
    vector->array = NULL;
    vector->size = 0;
    vector->capacity = 0;
}

void initPollFdVector(pollFdVector *vector)
{
    vector->size = 0;
    vector->capacity = INITIAL_CAPACITY;
    vector->array = (pollfd *)malloc(vector->capacity * sizeof(pollfd));
}

bool resizePollFdVector(pollFdVector *vec, size_t new_size, int value)
{
    if (new_size <= vec->size)
    {
        vec->size = new_size;
        return true;
    }

    if (new_size > vec->capacity)
    {
        size_t new_capacity;
        if (vec->capacity == 0)
        {
            new_capacity = new_size;
        }
        else
        {
            new_capacity = vec->capacity * 2;
            if (new_capacity < new_size)
            {
                new_capacity = new_size;
            }
        }

        pollfd *new_array = (pollfd *)realloc(vec->array, new_capacity * sizeof(pollfd));
        if (new_array == NULL)
        {
            return false;
        }

        vec->array = new_array;
        vec->capacity = new_capacity;
    }

    for (size_t i = vec->size; i < new_size; ++i)
    {
        vec->array[i] = (pollfd){};
    }

    vec->size = new_size;
    return true;
}

void clearPollFdVector(pollFdVector *vector)
{
    if (!vector)
        return;

    vector->size = 0; // Reset size to indicate an empty vector
}

void pollVectorPushBack(pollFdVector *vector, pollfd value)
{
    if (vector->size >= vector->capacity)
    {
        resizePollFdVector(vector, vector->size + 1, 0);
    }
    vector->array[vector->size++] = value;
}