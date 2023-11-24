#pragma once

#include <stddef.h>
#include <stdint.h>

#define BUFFER_SIZE 256

struct ring_buffer {
    char buffer[BUFFER_SIZE];
    size_t left;
    size_t right;
};

struct protocol_interface {
    struct ring_buffer volatile *buffers;
    size_t current_buffer;
};

size_t protocol_read(struct protocol_interface *interface, char *buffer,
                     size_t size);
size_t protocol_write(struct protocol_interface *interface, const char *buffer,
                      size_t size);
