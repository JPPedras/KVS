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

#define LOCAL_SERVER_ADDR "/tmp/local_server_address"
#define MAX_LENGTH 20

Group *group_head = NULL;

Group *create_group(char *group_id) {
    Group *new_group = malloc(sizeof(Group));
    new_group->table = create_table(CAPACITY);
    new_group->group_id = malloc(MAX_LENGTH * sizeof(char));
    strcpy(new_group->group_id, group_id);
    new_group->apps_head = NULL;
    new_group->next = group_head;
    group_head = new_group;

    return new_group;
}

void *com_thread(void *arg) {
    int n_bytes, k, flag, i, app_sock;
    int *pos = (int *)arg;
    char *value_ = (char *)malloc(sizeof(char) * MAX_LENGTH);
    char *key = (char *)malloc(sizeof(char) * MAX_LENGTH);
    char *value = (char *)malloc(sizeof(char) * MAX_LENGTH);
    Group *group = group_head;

    for (i = 0; i < *pos; i++) {
        group = group->next;
    }
    App *app = group->apps_head;
    app_sock = app->app_sock;

    while (1) {
        n_bytes = recv(app_sock, &flag, sizeof(int), 0);
        switch (flag) {
            case 0:
                n_bytes = recv(app_sock, key, sizeof(key), 0);
                n_bytes = recv(app_sock, value, sizeof(value), 0);
                ht_insert(group->table, key, value);
                break;
            case 1:
                n_bytes = recv(app_sock, key, sizeof(key), 0);
                // printf("vai entrar no search\n");
                value_ = ht_search(group->table, key);
                // printf("return: %s\n", value_);
                send(app_sock, value_, sizeof(value), 0);
                break;
            case 2:
                n_bytes = recv(app_sock, key, sizeof(key), 0);
                delete_item(group->table, key);
                break;
            case 3:
                app->conected = 0;
                pthread_exit(NULL);
                break;
        }
    }
}

void *accept_thread(void *arg) {
    int *server_sock = (int *)arg;
    int n_bytes, i = 0, aux_pos, flag = 0;
    pthread_t thread[10];
    int app_sock;
    char group_id[MAX_LENGTH];
    char secret[MAX_LENGTH];
    char confirmation[MAX_LENGTH];
    char msg[100];
    struct sockaddr_in auth_server_addr;
    int auth_addr_size = sizeof(auth_server_addr);
    auth_server_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &auth_server_addr.sin_addr);
    auth_server_addr.sin_port = htons(8080);
    Group *aux_group;

    while (1) {
        app_sock = accept(server_sock[0], NULL, NULL);

        n_bytes = recv(app_sock, group_id, sizeof(group_id), 0);
        sendto(server_sock[1], &flag, sizeof(flag), MSG_CONFIRM,
               (const struct sockaddr *)&auth_server_addr, auth_addr_size);
        sendto(server_sock[1], group_id, sizeof(group_id), MSG_CONFIRM,
               (const struct sockaddr *)&auth_server_addr, auth_addr_size);

        n_bytes =
            recvfrom(server_sock[1], confirmation, sizeof(confirmation), 0,
                     (struct sockaddr *)&auth_server_addr, &auth_addr_size);
        printf("confirmation msg: %s\n", confirmation);

        // n_bytes = recv(clients[i], group_id, sizeof(group_id), 0);
        sprintf(msg, "You are now connected to group %s", group_id);
        send(app_sock, msg, sizeof(msg), 0);
        // if authorized
        int pos = 0;
        aux_group = group_head;

        while (aux_group != NULL) {
            if (strcmp(aux_group->group_id, group_id) == 0) {
                break;
            }
            aux_group = aux_group->next;
            pos++;
        }
        printf("pos:%d\n", pos);
        App *new_app = malloc(sizeof(App));
        new_app->app_sock = app_sock;
        new_app->conected = 1;
        new_app->next = aux_group->apps_head;
        aux_group->apps_head = new_app;
        aux_pos = pos;
        pthread_create(&thread[i], NULL, com_thread, &aux_pos);
        pos = 0;
    }
}

int main() {
    char *command = (char *)malloc(MAX_LENGTH * sizeof(char));
    int server_sock[2], recv_sock[10], i = 0, flag;
    struct sockaddr_un un_server_sock_addr;
    struct sockaddr_in in_server_sock_addr;
    struct sockaddr_in auth_server_addr;
    int un_server_addr_size = sizeof(un_server_sock_addr);
    int in_server_addr_size = sizeof(in_server_sock_addr);
    pthread_t ac_thread;
    int n_bytes;
    int auth_addr_size = sizeof(auth_server_addr);
    auth_server_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &auth_server_addr.sin_addr);
    auth_server_addr.sin_port = htons(8080);

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

    /*in_server_sock_addr.sin_family = AF_INET;
    in_server_sock_addr.sin_addr.s_addr = INADDR_ANY;
    in_server_sock_addr.sin_port = htons(8081);

    if (bind(server_sock[1], (struct sockaddr *)&in_server_sock_addr,
             in_server_addr_size) == -1) {
        printf("erro no bind2\n");
        exit(-1);
    }*/

    listen(server_sock[0], 2);

    pthread_create(&ac_thread, NULL, accept_thread, &server_sock);
    while (1) {
        printf("Possible commands:\n");
        printf("c \'group_id\' -> creates group \n");
        printf("d \'group_id\' -> deletes group\n");
        printf("i \'group_id\' -> shows group info\n");
        printf("s \'group_id\' -> gives status of apps\n");
        fgets(command, MAX_LENGTH, stdin);
        char *token = strtok(command, " ");
        if (strcmp(token, "c") == 0) {
            flag = 1;
            token = strtok(NULL, " ");
            token[strcspn(token, "\n")] = 0;
            sendto(server_sock[1], &flag, sizeof(flag), MSG_CONFIRM,
                   (const struct sockaddr *)&auth_server_addr, auth_addr_size);
            sendto(server_sock[1], token, sizeof(token), MSG_CONFIRM,
                   (const struct sockaddr *)&auth_server_addr, auth_addr_size);
            /*n_bytes=recv_from(server_sock[1], token, sizeof(token),
               MSG_CONFIRM, (const struct sockaddr *)&auth_server_addr,
               auth_addr_size);*/
            Group *group = create_group(token);
        } else if (strcmp(token, "d") == 0) {
            flag = 2;
            token = strtok(NULL, " ");
            token[strcspn(token, "\n")] = 0;
            sendto(server_sock[1], &flag, sizeof(flag), MSG_CONFIRM,
                   (const struct sockaddr *)&auth_server_addr, auth_addr_size);
            sendto(server_sock[1], token, sizeof(token), MSG_CONFIRM,
                   (const struct sockaddr *)&auth_server_addr, auth_addr_size);

        } else if (strcmp(token, "i") == 0) {
            flag = 2;
            token = strtok(NULL, " ");
            token[strcspn(token, "\n")] = 0;
            sendto(server_sock[1], &flag, sizeof(flag), MSG_CONFIRM,
                   (const struct sockaddr *)&auth_server_addr, auth_addr_size);
            sendto(server_sock[1], token, sizeof(token), MSG_CONFIRM,
                   (const struct sockaddr *)&auth_server_addr, auth_addr_size);
        }
        remove(LOCAL_SERVER_ADDR);

        return 0;
    }