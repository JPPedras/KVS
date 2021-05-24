#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "lib.h"

#define SERVER_ADDR "/tmp/server_address"
#define MAX_LENGTH 512

void *send_num(void *arg) {
    int *client_sock = (int *)arg;
    char msg[100];

    while (1) {
        fgets(msg, 100, stdin);
        send(*client_sock, msg, sizeof(msg), 0);
        // printf("client sent: %s", msg);
        // printf("sent: %d\n", num);
        // sleep(1);
    }
    pthread_exit(NULL);
}

void *recv_prime(void *arg) {
    int n_bytes;
    char msg[100];
    int *client_sock = (int *)arg;

    while (1) {
        n_bytes = recv(*client_sock, msg, sizeof(msg), 0);
        printf("new msg: %s", msg);
    }
    pthread_exit(NULL);
}

int main() {
    int flag = establish_connection("111", "password");
    char *key = (char *)malloc(sizeof(char) * MAX_LENGTH);
    char *value = (char *)malloc(sizeof(char) * MAX_LENGTH);

    strcpy(key, "nome");
    strcpy(value, "goncalo");
    flag = put_value(key, value);

    strcpy(key, "apelido");
    strcpy(value, "coelho");
    flag = put_value(key, value);

    strcpy(key, "apelido");
    strcpy(value, "random");
    flag = get_value(key, &value);
    printf("apelido: %s\n", value);

    flag = close_connection();
    getchar();
    return 0;
}