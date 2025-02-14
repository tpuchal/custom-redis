#include "uint8tvector.h"
#include <stdio.h>
#include<string.h>
#include <sys/errno.h>

void freeVector(uint8TVector *vec)
{
    free(vec->array);
    vec->array = NULL;
    vec->size = 0;
    vec->capacity = 0;
}

void initVector(uint8TVector *vector)
{
    vector->size = 0;
    vector->capacity = INITIAL_CAPACITY;
    vector->array = (uint8_t *)malloc(vector->capacity * sizeof(uint8_t));
    if (!vector->array)
    {
        fprintf(stderr, "[%d] %s\n", errno, "memory allocation failed \n");
        abort();
    }
}

bool resizeVector(uint8TVector *vec, size_t new_size, uint8_t value)
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

        uint8_t *new_array = (uint8_t *)realloc(vec->array, new_capacity * sizeof(uint8_t));
        if (new_array == NULL)
        {
            return false;
        }

        vec->array = new_array;
        vec->capacity = new_capacity;
    }

    for (size_t i = vec->size; i < new_size; ++i)
    {
        vec->array[i] = value;
    }

    vec->size = new_size;
    return true;
}

void appendToVector(uint8TVector *vector, const uint8_t *value, size_t len)
{
    size_t oldSize = vector->size;

    size_t check = vector->size + vector->capacity;

    if (len > check)
    {
        resizeVector(vector, vector->size + len, 0);
    }

    for (int i = 0; i < len; i++)
    {
        vector->array[oldSize + i] = value[i];
    }
    vector->size += len;
}

void eraseFromVector(uint8TVector *vector, size_t index)
{
    if (index >= vector->size || index < 0)
    {
        fprintf(stderr, "[%d] %s\n", errno, "Failed to erase from vector\n");
        abort();
    }
    memmove(&vector->array[index], &vector->array[index + 1], (vector->size - index - 1) * sizeof(uint8_t));
    vector->size--;
}