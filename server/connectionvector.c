#include "connectionvector.h"
#include "connection.h"

ConnectionVector initConnectionVector()
{
    ConnectionVector retConnVector;
    retConnVector.size = 0;
    retConnVector.capacity = 1;
    retConnVector.array = (Connection *)malloc(sizeof(Connection));

    return retConnVector;
}

void freeConnectionVector(ConnectionVector *vector)
{
    free(vector->array);
    vector->array = NULL;
    vector->size = 0;
    vector->capacity = 0;
}

bool resizeConnectionVector(ConnectionVector *vec, size_t new_size, int value)
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

        Connection *new_array = (Connection *)realloc(vec->array, new_capacity * sizeof(Connection));
        if (new_array == NULL)
        {
            return false;
        }

        vec->array = new_array;
        vec->capacity = new_capacity;
    }

    for (size_t i = vec->size; i < new_size; ++i)
    {
        vec->array[i] = (Connection)emptyConnection();
    }

    vec->size = new_size;
    return true;
}