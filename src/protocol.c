#define _GNU_SOURCE
#include "protocol.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

// static void print_state(struct protocol_interface *interface) {
//     printf("current_buffer = %zu\n", interface->current_buffer);
//     printf("first buffer:\n");
//     printf("left = %zu\n", interface->buffers[0].left);
//     printf("right = %zu\n", interface->buffers[0].right);
//     printf("second buffer:\n");
//     printf("left = %zu\n", interface->buffers[1].left);
//     printf("right = %zu\n", interface->buffers[1].right);
// }

static void swap_buffers(struct protocol_interface *interface) {
    interface->current_buffer = (interface->current_buffer + 1) % 2;
}

static size_t min(size_t a, size_t b) { return a < b ? a : b; }

size_t protocol_read(struct protocol_interface *interface, char *buffer,
                     size_t size) {
    size_t read = 0;
    size_t current = interface->buffers[interface->current_buffer].left;

    while (read < size) {
        if (current == BUFFER_SIZE) {
            interface->buffers[interface->current_buffer].right = 0;
            swap_buffers(interface);
            struct ring_buffer volatile *next =
                &interface->buffers[interface->current_buffer];
            while (next->left != 0) {
                usleep(1000);
            }
            current = 0;
        }

        while (current == interface->buffers[interface->current_buffer].right) {
            usleep(1000);
        }

        size_t written =
            strnlen(
                (char *)interface->buffers[interface->current_buffer].buffer +
                    current,
                interface->buffers[interface->current_buffer].right - current -
                    1) +
            1;

        memcpy(
            buffer + read,
            (const char *)interface->buffers[interface->current_buffer].buffer +
                current,
            written);

        read += written;
        current += written;

        if (*(buffer + read - 1) == '\0') {
            break;
        }
    }

    interface->buffers[interface->current_buffer].left = current;
    return read;
}

size_t protocol_write(struct protocol_interface *interface, const char *buffer,
                      size_t size) {
    size_t written = 0;
    size_t current = interface->buffers[interface->current_buffer].right;

    while (written < size) {
        size_t read = strnlen(buffer + written,
                              min(BUFFER_SIZE - current, size - written) - 1) +
                      1;

        memcpy((char *)interface->buffers[interface->current_buffer].buffer +
                   current,
               buffer + written, read);

        written += read;
        current += read;

        if (current == BUFFER_SIZE) {
            interface->buffers[interface->current_buffer].right = BUFFER_SIZE;
            swap_buffers(interface);
            while (interface->buffers[interface->current_buffer].right != 0) {
                usleep(1000);
            }
            interface->buffers[interface->current_buffer].left = 0;
            current = 0;
        }

        if (*(buffer + written - 1) == '\0') {
            break;
        }
    }

    interface->buffers[interface->current_buffer].right = current;
    return written;
}
