#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "lib.h"

#define LOCAL_SERVER_ADDRESS "/tmp/local_server_address"
#define MAX_LENGTH 512
int app_sock;

int establish_connection(char* group_id, char* secret) {
    struct sockaddr_un app_sock_addr;
    int app_addr_size = sizeof(app_sock_addr);
    struct sockaddr_un local_server_sock_addr;
    int server_addr_size = sizeof(local_server_sock_addr);
    int n_bytes, flag;

    app_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (app_sock == -1) {
        printf("fail 1\n");
        exit(-1);
    }

    memset(&local_server_sock_addr, 0, sizeof(struct sockaddr_un));
    local_server_sock_addr.sun_family = AF_UNIX;
    strcpy(local_server_sock_addr.sun_path, LOCAL_SERVER_ADDRESS);
    app_sock_addr.sun_family = AF_UNIX;
    sprintf(app_sock_addr.sun_path, "/tmp/app_socket_%d", getpid());

    unlink(app_sock_addr.sun_path);
    if (bind(app_sock, (struct sockaddr*)&app_sock_addr, app_addr_size) == -1) {
        printf("fail 2\n");
        exit(-2);
    }

    if (connect(app_sock, (struct sockaddr*)&local_server_sock_addr,
                sizeof(struct sockaddr_un)) == -1) {
        perror("erro de connect:");
    }
    send(app_sock, group_id, MAX_LENGTH, 0);
    send(app_sock, secret, MAX_LENGTH, 0);
    n_bytes = recv(app_sock, &flag, sizeof(int), 0);
    if (flag == 1) {
        return 0;
    } else {
        return -1;
    }
}
int put_value(char* key, char* value) {
    int flag = 0;
    int size = strlen(value) + 1;
    send(app_sock, &flag, sizeof(int), 0);
    send(app_sock, key, MAX_LENGTH, 0);
    send(app_sock, &size, sizeof(int), 0);
    send(app_sock, value, strlen(value) + 1, 0);
    return 1;
}

int get_value(char* key, char** value) {
    int flag = 1, n_bytes;
    int size;
    send(app_sock, &flag, sizeof(int), 0);
    send(app_sock, key, MAX_LENGTH, 0);
    n_bytes = recv(app_sock, &flag, sizeof(int), 0);
    if (flag == 1) {
        n_bytes = recv(app_sock, &size, sizeof(int), 0);
        *value = malloc(size * sizeof(char));
        n_bytes = recv(app_sock, *value, size, 0);
        return 1;
    } else {
        return -1;
    }
}
int delete_value(char* key) {
    int flag = 2, n_bytes;
    send(app_sock, &flag, sizeof(int), 0);
    send(app_sock, key, MAX_LENGTH, 0);
    n_bytes = recv(app_sock, &flag, sizeof(int), 0);
    if (flag == 1) {
        return 1;
    } else {
        return -1;
    }
}
int close_connection() {
    int flag = 3, n_bytes;
    send(app_sock, &flag, sizeof(int), 0);
    close(app_sock);
    return 1;
}
