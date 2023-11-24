#pragma once

enum connection_status {
    CONNECTION_OK,
    CONNECTION_RETRY,
    CONNECTION_ERROR,
};
enum connection_status connect_as_server(int *fd);
enum connection_status connect_as_client(int fd);
