#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "lib.h"

#define LOCAL_SERVER_ADDRESS "/tmp/local_server_address"
#define LOCAL_SERVER_ADDRESS_CB "/tmp/local_server_address_cb"
#define MAX_LENGTH 512
int app_sock[2];
int callback_activate = 0;
/*
return  0-> success
return -1 -> group does not exist
return -2 -> incorrect password
return -3 -> erro de conexão
return -4 -> erro de memória
return -5 -> timeout
*/
int establish_connection(char* group_id, char* secret) {
    struct sockaddr_un app_sock_addr[2];
    int app_addr_size = sizeof(app_sock_addr[0]);
    struct sockaddr_un local_server_sock_addr[2];
    int server_addr_size = sizeof(local_server_sock_addr[0]);
    int n_bytes, flag;

    for (int i = 0; i < 2; i++) {
        app_sock[i] = socket(AF_UNIX, SOCK_STREAM, 0);
        if (app_sock[i] == -1) {
            return -3;
        }
    }
    memset(&local_server_sock_addr[0], 0, sizeof(struct sockaddr_un));
    local_server_sock_addr[0].sun_family = AF_UNIX;
    strcpy(local_server_sock_addr[0].sun_path, LOCAL_SERVER_ADDRESS);
    memset(&local_server_sock_addr[1], 0, sizeof(struct sockaddr_un));
    local_server_sock_addr[1].sun_family = AF_UNIX;
    strcpy(local_server_sock_addr[1].sun_path, LOCAL_SERVER_ADDRESS_CB);
    app_sock_addr[0].sun_family = AF_UNIX;
    sprintf(app_sock_addr[0].sun_path, "/tmp/app_socket_%d", getpid());
    app_sock_addr[1].sun_family = AF_UNIX;
    sprintf(app_sock_addr[1].sun_path, "/tmp/callback_socket_%d", getpid());

    unlink(app_sock_addr[0].sun_path);
    if (bind(app_sock[0], (struct sockaddr*)&app_sock_addr[0], app_addr_size) ==
        -1) {
        return -3;
    }
    unlink(app_sock_addr[1].sun_path);
    if (bind(app_sock[1], (struct sockaddr*)&app_sock_addr[1], app_addr_size) ==
        -1) {
        return -3;
    }

    if (connect(app_sock[0], (struct sockaddr*)&local_server_sock_addr[0],
                sizeof(struct sockaddr_un)) == -1) {
        return -3;
    }
    n_bytes = send(app_sock[0], group_id, MAX_LENGTH, 0);
    if (n_bytes == -1) {
        return -3;
    }
    n_bytes = send(app_sock[0], secret, MAX_LENGTH, 0);
    if (n_bytes == -1) {
        return -3;
    }
    n_bytes = recv(app_sock[0], &flag, sizeof(int), 0);
    if (n_bytes == -1) {
        return -3;
    }

    if (flag == 0) {
        if (connect(app_sock[1], (struct sockaddr*)&local_server_sock_addr[1],
                    sizeof(struct sockaddr_un)) == -1) {
            return -3;
        }
    }
    return flag;
}

/*
return 1 -> success
return -1 -> group does no longer exist
return -3 -> erro de conexão
return -4 -> erro de memória
*/
int put_value(char* key, char* value) {
    int flag = 0, n_bytes;
    int size = strlen(value) + 1;

    n_bytes = send(app_sock[0], &flag, sizeof(int), 0);
    if (n_bytes == -1) {
        return -3;
    }
    n_bytes = recv(app_sock[0], &flag, sizeof(int), 0);
    if (flag == -1 || n_bytes == 0) {
        return -1;
    } else if (n_bytes == -1) {
        return -3;
    }
    n_bytes = send(app_sock[0], key, MAX_LENGTH, 0);
    if (n_bytes == -1) {
        return -3;
    }
    n_bytes = send(app_sock[0], &size, sizeof(int), 0);
    if (n_bytes == -1) {
        return -3;
    }
    n_bytes = send(app_sock[0], value, strlen(value) + 1, 0);
    if (n_bytes == -1) {
        return -3;
    }
    n_bytes = recv(app_sock[0], &flag, sizeof(int), 0);
    if (n_bytes == -1) {
        return -3;
    }

    return flag;
}

/*
return 1 -> success
return -1 -> group does no longer exist
return -2 -> key does not exist
return -3 -> erro de conexão
return -4 -> erro de memória
*/
int get_value(char* key, char** value) {
    int flag = 1, n_bytes, size;

    n_bytes = send(app_sock[0], &flag, sizeof(int), 0);
    if (n_bytes == -1) {
        return -3;
    }
    n_bytes = recv(app_sock[0], &flag, sizeof(int), 0);
    if (flag == -1 || n_bytes == 0) {
        return -1;
    } else if (n_bytes == -1) {
        return -3;
    }
    n_bytes = send(app_sock[0], key, MAX_LENGTH, 0);
    if (n_bytes == -1) {
        return -3;
    }
    n_bytes = recv(app_sock[0], &flag, sizeof(int), 0);
    if (n_bytes == -1) {
        return -3;
    }
    if (flag == 1) {
        n_bytes = recv(app_sock[0], &size, sizeof(int), 0);
        if (n_bytes == -1) {
            return -3;
        }
        *value = malloc(size * sizeof(char));
        if (*value == NULL) {
            return -4;
        }
        n_bytes = recv(app_sock[0], *value, size, 0);
        if (n_bytes == -1) {
            return -3;
        }
    }
    return flag;
}

/*
return 1 -> success
return -1 -> group does no longer exist
return -2 -> key does not exist
return -3 -> erro de conexão
return -4 -> erro de memória
*/
int delete_value(char* key) {
    int flag = 2, n_bytes;

    n_bytes = send(app_sock[0], &flag, sizeof(int), 0);
    if (n_bytes == -1) {
        return -3;
    }
    n_bytes = recv(app_sock[0], &flag, sizeof(int), 0);
    if (flag == -1 || n_bytes == 0) {
        return -1;
    } else if (n_bytes == -1) {
        return -3;
    }
    n_bytes = send(app_sock[0], key, MAX_LENGTH, 0);
    if (n_bytes == -1) {
        return -3;
    }
    n_bytes = recv(app_sock[0], &flag, sizeof(int), 0);
    if (n_bytes == -1) {
        return -3;
    }
    return flag;
}

/*
return 1 -> success
return -1 -> group no longer exists
return -3 -> erro de conexão
return -4 -> erro de memória
*/
int close_connection() {
    int flag = 3, n_bytes;

    n_bytes = send(app_sock[0], &flag, sizeof(int), 0);
    if (n_bytes == -1) {
        return -3;
    }
    n_bytes = recv(app_sock[0], &flag, sizeof(int), 0);
    if (n_bytes == -1) {
        return -3;
    }
    if (close(app_sock[0]) == -1) {
        return -3;
    }
    if (close(app_sock[1]) == -1) {
        return -3;
    }
    callback_activate = 0;
    return flag;
}

/*
return 1 -> success
return -1 -> group no longer exists
return -2 -> key does not exist
return -3 -> erro de conexão
return -4 -> erro de memória
return -6 -> erro no fork
*/
int register_callback(char* key, void (*callback_function)(char*)) {
    int flag = 4, n_bytes = 1;
    pid_t childPid;
    char* changed_key = malloc(MAX_LENGTH * sizeof(char));
    if (changed_key == NULL) {
        return -4;
    }

    n_bytes = send(app_sock[0], &flag, sizeof(int), 0);
    if (n_bytes == -1) {
        return -3;
    }
    n_bytes = recv(app_sock[0], &flag, sizeof(int), 0);
    if (flag == -1 || n_bytes == 0) {
        return -1;
    } else if (n_bytes == -1) {
        return -3;
    }
    n_bytes = send(app_sock[0], key, MAX_LENGTH, 0);
    if (n_bytes == -1) {
        return -3;
    }
    n_bytes = recv(app_sock[0], &flag, sizeof(int), 0);
    if (n_bytes == -1) {
        return -3;
    }
    if (flag == 1 && callback_activate == 0) {
        callback_activate = 1;
        switch (childPid = fork()) {
            case 0:
                while (n_bytes != 0) {
                    n_bytes = recv(app_sock[1], changed_key, MAX_LENGTH, 0);
                    if (n_bytes == -1) {
                        exit(-3);
                    }
                    // printf("n_bytes: %d\n", n_bytes);
                    if (n_bytes > 0) {
                        callback_function(changed_key);
                    }
                }
            case -1:
                return -6;
        }
    }
    return flag;
}
