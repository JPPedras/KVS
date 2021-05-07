#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "hash_tables.h"

int clients[10] = {0};
#define LOCAL_SERVER_ADDR "/tmp/local_server_address"
#define MAX_LENGTH 20

void *com_thread(void *arg) {
    int n_bytes, k, flag;
    int *recv_sock = (int *)arg;
    char *value_ = (char *)malloc(sizeof(char) * MAX_LENGTH);
    Group *group = create_table(CAPACITY);
    char *key = (char *)malloc(sizeof(char) * MAX_LENGTH);
    char *value = (char *)malloc(sizeof(char) * MAX_LENGTH);

    while (1) {
        n_bytes = recv(*recv_sock, &flag, sizeof(int), 0);
        switch (flag) {
            case 0:
                n_bytes = recv(*recv_sock, key, sizeof(key), 0);
                n_bytes = recv(*recv_sock, value, sizeof(value), 0);
                ht_insert(group, key, value);
                break;
            case 1:
                n_bytes = recv(*recv_sock, key, sizeof(key), 0);
                // printf("vai entrar no search\n");
                value_ = ht_search(group, key);
                // printf("return: %s\n", value_);
                send(*recv_sock, value_, sizeof(value), 0);
                break;
            case 2:
                n_bytes = recv(*recv_sock, key, sizeof(key), 0);
                delete_item(group, key);
                break;
            case 3:
                pthread_exit(NULL);
                break;
        }
    }
}

int main() {
    char group[10];
    char msg[100];
    int server_sock, recv_sock[10], i = 0;
    struct sockaddr_un server_sock_addr;
    int server_addr_size = sizeof(server_sock_addr);
    pthread_t thread[10];
    int n_bytes;

    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock == -1) {
        printf("fail to create socket\n");
        exit(-1);
    }
    server_sock_addr.sun_family = AF_UNIX;
    strcpy(server_sock_addr.sun_path, LOCAL_SERVER_ADDR);

    unlink(LOCAL_SERVER_ADDR);
    if (bind(server_sock, (struct sockaddr *)&server_sock_addr,
             server_addr_size) == -1) {
        exit(-1);
    }
    listen(server_sock, 2);

    while (1) {
        clients[i] = accept(server_sock, NULL, NULL);
        n_bytes = recv(clients[i], group, sizeof(group), 0);
        sprintf(msg, "You are now connected to group %s", group);
        send(clients[i], msg, sizeof(msg), 0);
        pthread_create(&thread[i], NULL, com_thread, &clients[i]);
        i++;
    }
    remove(LOCAL_SERVER_ADDR);

    return 0;
}