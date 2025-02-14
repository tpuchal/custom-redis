#ifndef CONNECTION_VECTOR_HEADER
#define CONNECTION_VECTOR_HEADER
#include "connection.h"

typedef struct
{
    int size;
    int capacity;
    Connection *array;

} ConnectionVector;

ConnectionVector initConnectionVector();

void freeConnectionVector(ConnectionVector *vector);

bool resizeConnectionVector(ConnectionVector *vec, size_t new_size, int value);

#endif