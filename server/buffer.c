#include "buffer.h"
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>

bool consumeNewBuffer(Buffer *buffer, size_t data_size)
{
    size_t current_data_size = buffer->data_end - buffer->data_begin;
    if (data_size > current_data_size)
    {
        return false;
    }
    buffer->data_begin += data_size;

    if (buffer->data_begin == buffer->data_end)
    {
        buffer->data_begin = buffer->buffer_begin;
        buffer->data_end = buffer->buffer_begin;
    }

    return true;
}


bool appendToNewBuffer(Buffer *buffer, const uint8_t *data, size_t data_size)
{
    size_t available_space = buffer->buffer_end - buffer->data_end;
    if (data_size > available_space)
    {
        return false;
    }

    memcpy(buffer->data_end, data, data_size);
    buffer->data_end += data_size;

    return true;
}

void initBuffer(Buffer *buffer)
{
    uint8_t buffer_size[MAX_BUFFER_SIZE];
    buffer->buffer_begin = buffer_size;
    buffer->buffer_end = buffer_size + sizeof(buffer_size);
    buffer->data_begin = buffer_size;
    buffer->data_end = buffer_size;
}