#include "connection.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "socket.h"

int bind_to_socket(void) {
    struct sockaddr_un name;

    int connection_socket = socket(PF_LOCAL, SOCK_STREAM, 0);

    assert(connection_socket != -1);

    memset(&name, 0, sizeof(name));
    name.sun_family = PF_LOCAL;
    strncpy(name.sun_path, SOCKET_NAME, sizeof(name.sun_path) - 1);

    int ret =
        bind(connection_socket, (const struct sockaddr *)&name, sizeof(name));

    if (ret == -1)
        return -1;

    assert(chmod(SOCKET_NAME, 0777) != -1);
    return connection_socket;
}

int connect_to_socket(void) {
    struct sockaddr_un name;

    int connection_socket = socket(PF_LOCAL, SOCK_STREAM, 0);

    assert(connection_socket != -1);

    memset(&name, 0, sizeof(name));
    name.sun_family = PF_LOCAL;
    strncpy(name.sun_path, SOCKET_NAME, sizeof(name.sun_path) - 1);

    int res = connect(connection_socket, (const struct sockaddr *)&name,
                      sizeof(name));

    if (res == -1)
        return -1;

    return connection_socket;
}

enum connection_status connect_as_server(int *fd) {
    int sock_conn = bind_to_socket();

    if (sock_conn == -1)
        return CONNECTION_RETRY;

    int8_t data;
    struct iovec iov;
    struct msghdr msgh;

    union {
        char buf[CMSG_SPACE(sizeof(int))];
        struct cmsghdr allign;
    } control_msg;

    struct cmsghdr *cmsgp;

    msgh.msg_name = NULL;
    msgh.msg_namelen = 0;

    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;
    iov.iov_base = &data;
    iov.iov_len = sizeof(data);

    msgh.msg_control = control_msg.buf;
    msgh.msg_controllen = sizeof(control_msg.buf);

    assert(listen(sock_conn, 1) != -1);

    int sockfd = accept(sock_conn, NULL, NULL);

    if (sockfd == -1) {
        close(sock_conn);
        unlink(SOCKET_NAME);
        printf("sockfd == -1\n");
        abort();
    }

    printf("waining for client\n");

    if (recvmsg(sockfd, &msgh, 0) == -1) {
        close(sockfd);
        close(sock_conn);
        unlink(SOCKET_NAME);
        printf("recvmsg == -1\n");
        abort();
    }

    printf("got data\n");
    printf("sending OK\n");

    if (write(sockfd, "OK", 2) == -1) {
        close(sockfd);
        close(sock_conn);
        unlink(SOCKET_NAME);
        printf("write == -1\n");
        abort();
    }

    close(sockfd);
    close(sock_conn);
    unlink(SOCKET_NAME);

    cmsgp = CMSG_FIRSTHDR(&msgh);

    assert(cmsgp != NULL && cmsgp->cmsg_len == CMSG_LEN(sizeof(int)) &&
           cmsgp->cmsg_type == SCM_RIGHTS && cmsgp->cmsg_level == SOL_SOCKET);

    assert(data == 123);

    printf("transfer successful\n");

    memcpy(fd, CMSG_DATA(cmsgp), sizeof(int));

    return CONNECTION_OK;
}

enum connection_status connect_as_client(int fd) {
    int sockfd = connect_to_socket();

    if (sockfd == -1)
        return CONNECTION_RETRY;

    printf("connected\n");

    int8_t data = 123;
    struct iovec iov;
    struct msghdr msgh;

    union {
        uint8_t buf[CMSG_SPACE(sizeof(int))];
        struct cmsghdr allign;
    } control_msg;

    struct cmsghdr *cmsgp;

    msgh.msg_name = NULL;
    msgh.msg_namelen = 0;

    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;
    iov.iov_base = &data;
    iov.iov_len = sizeof(data);

    msgh.msg_control = control_msg.buf;
    msgh.msg_controllen = sizeof(control_msg.buf);

    cmsgp = CMSG_FIRSTHDR(&msgh);
    cmsgp->cmsg_level = SOL_SOCKET;
    cmsgp->cmsg_type = SCM_RIGHTS;
    cmsgp->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsgp), &fd, sizeof(int));

    printf("sending data\n");

    if (sendmsg(sockfd, &msgh, 0) == -1) {
        close(sockfd);
        printf("sendmsg == -1\n");
        return CONNECTION_RETRY;
    }

    char buf[3] = {0};

    printf("waiting for OK\n");

    ssize_t res = read(sockfd, buf, 2);

    if (res == -1) {
        close(sockfd);
        printf("haven't received OK, retrying\n");
        return CONNECTION_RETRY;
    }

    close(sockfd);

    assert(strcmp(buf, "OK") == 0);

    printf("received OK\n");

    return CONNECTION_OK;
}
