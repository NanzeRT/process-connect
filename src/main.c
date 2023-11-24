#define _GNU_SOURCE

#include <assert.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include "connection.h"
#include "protocol.h"

enum connection_status server(void) {
    int fd = -1;

    printf("trying server\n");

    enum connection_status res = connect_as_server(&fd);

    if (res != CONNECTION_OK)
        return res;

    printf("server: fd = %d\n", fd);

    struct ring_buffer *buffers =
        mmap(NULL, sizeof(struct ring_buffer) * 2, PROT_READ | PROT_WRITE,
             MAP_SHARED, fd, 0);

    assert(buffers != MAP_FAILED);

    struct protocol_interface interface = {
        .buffers = buffers,
        .current_buffer = 0,
    };

    // server writes to client

    char buf[100] = {0};

    while (fgets(buf, 100, stdin) != NULL) {
        protocol_write(&interface, buf, 100);
    }

    protocol_write(&interface, "\0", 1);
    protocol_write(&interface, "\0", 1);

    close(fd);

    printf("server: closed fd\n");

    return CONNECTION_OK;
}

enum connection_status client(void) {
    int fd = memfd_create("test", 0);
    ftruncate(fd, sizeof(struct ring_buffer) * 2);

    printf("trying client\n");

    enum connection_status res = connect_as_client(fd);

    if (res != CONNECTION_OK) {
        close(fd);
        return res;
    }

    printf("client: fd = %d\n", fd);

    struct ring_buffer *buffers =
        mmap(NULL, sizeof(struct ring_buffer) * 2, PROT_READ | PROT_WRITE,
             MAP_SHARED, fd, 0);

    assert(buffers != MAP_FAILED);

    struct protocol_interface interface = {
        .buffers = buffers,
        .current_buffer = 0,
    };

    // client reads from server

    char buf[100] = {0};
    char prev = '\0';

    while (1) {
        protocol_read(&interface, buf, 99);

        printf("%s", buf);

        if (buf[0] == '\0' && prev == '\0')
            break;
        prev = buf[0];
    }

    printf("\n");

    printf("client: closing fd\n");

    close(fd);

    return CONNECTION_OK;
}

static const int RETRIES = 10;

int main(int argc, char **argv) {
    for (int i = 0; i < RETRIES; i++) {
        enum connection_status res = server();
        switch (res) {
        case CONNECTION_OK:
            return 0;
        case CONNECTION_ERROR:
            printf("errored\n");
            return -1;
        case CONNECTION_RETRY:
            break;
        }

        res = client();
        switch (res) {
        case CONNECTION_OK:
            return 0;
        case CONNECTION_ERROR:
            printf("errored\n");
            return -1;
        case CONNECTION_RETRY:
            break;
        }
    }

    printf("max retries exceeded\n");
    return -1;
}
