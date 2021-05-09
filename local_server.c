#include <arpa/inet.h>
#include <netinet/in.h>
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

void *accept_thread(void *arg) {
    int *server_sock = (int *)arg;
    int n_bytes, i = 0;
    pthread_t thread[10];
    char group_id[MAX_LENGTH];
    char secret[MAX_LENGTH];
    char confirmation[MAX_LENGTH];
    char msg[100];
    struct sockaddr_in auth_server_addr;
    int auth_addr_size = sizeof(auth_server_addr);
    auth_server_addr.sin_family = AF_INET;
    auth_server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    auth_server_addr.sin_port = htons(8080);
    // inet_aton("127.0.0.1", &auth_server_addr.sin_addr);

    while (1) {
        clients[i] = accept(server_sock[0], NULL, NULL);

        n_bytes = recv(clients[i], group_id, sizeof(group_id), 0);
        sendto(server_sock[1], group_id, sizeof(group_id), MSG_CONFIRM,
               (const struct sockaddr *)&auth_server_addr, auth_addr_size);
        printf("enviu\n");

        n_bytes =
            recvfrom(server_sock[1], confirmation, sizeof(confirmation), 0,
                     (struct sockaddr *)&auth_server_addr, &auth_addr_size);
        printf("confirmation msg: %s\n", confirmation);
        // n_bytes = recv(clients[i], group_id, sizeof(group_id), 0);
        sprintf(msg, "You are now connected to group %s", group_id);
        send(clients[i], msg, sizeof(msg), 0);
        pthread_create(&thread[i], NULL, com_thread, &clients[i]);
        i++;
    }
}

int main() {
    char *command = (char *)malloc(MAX_LENGTH * sizeof(char));
    int server_sock[2], recv_sock[10], i = 0;
    struct sockaddr_un un_server_sock_addr;
    struct sockaddr_in in_server_sock_addr;
    struct sockaddr_in auth_server_addr;
    int un_server_addr_size = sizeof(un_server_sock_addr);
    int in_server_addr_size = sizeof(in_server_sock_addr);
    pthread_t ac_thread;
    int n_bytes;

    server_sock[0] = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock[0] == -1) {
        printf("fail to create socket\n");
        exit(-1);
    }
    server_sock[1] = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sock[1] == -1) {
        printf("fail to create socket\n");
        exit(-1);
    }
    un_server_sock_addr.sun_family = AF_UNIX;
    strcpy(un_server_sock_addr.sun_path, LOCAL_SERVER_ADDR);

    unlink(LOCAL_SERVER_ADDR);
    if (bind(server_sock[0], (struct sockaddr *)&un_server_sock_addr,
             un_server_addr_size) == -1) {
        printf("erro no bind1\n");
        exit(-1);
    }

    listen(server_sock[0], 2);

    pthread_create(&ac_thread, NULL, accept_thread, &server_sock);
    while (1) {
        fgets(command, MAX_LENGTH, stdin);
        printf("u want to %s\n", command);
    }
    remove(LOCAL_SERVER_ADDR);

    return 0;
}