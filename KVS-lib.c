#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "lib.h"

#define LOCAL_SERVER_ADDRESS "/tmp/local_server_address"
int app_sock;

int establish_connection(char* group_id, char* secret) {
    struct sockaddr_un app_sock_addr;
    int app_addr_size = sizeof(app_sock_addr);
    struct sockaddr_un local_server_sock_addr;
    int server_addr_size = sizeof(local_server_sock_addr);
    int n_bytes;
    char msg[100];

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
    // sleep(1);
    send(app_sock, group_id, sizeof(group_id), 0);

    n_bytes = recv(app_sock, msg, sizeof(msg), 0);

    printf("%s\n", msg);

    return 0;
}
int put_value(char* key, char* value) {
    int flag = 0;
    send(app_sock, &flag, sizeof(int), 0);
    send(app_sock, key, sizeof(key), 0);
    send(app_sock, value, sizeof(value), 0);
    return 0;
}

int get_value(char* key, char** value) {
    int flag = 1;
    send(app_sock, &flag, sizeof(int), 0);
    send(app_sock, key, sizeof(key), 0);
    recv(app_sock, *value, sizeof(value), 0);
    return 0;
}
int delete_value(char* key) {
    int flag = 2;
    send(app_sock, &flag, sizeof(int), 0);
    send(app_sock, key, sizeof(key), 0);
    return 0;
}
int close_connection() {
    int flag = 3;
    send(app_sock, &flag, sizeof(int), 0);
    return 0;
}
