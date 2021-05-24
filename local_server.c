#include "hash_tables.h"
#include "lib.h"

#define LOCAL_SERVER_ADDR "/tmp/local_server_address"
#define MAX_LENGTH 512

Group *group_head = NULL;

void create_group(char *group_id) {
    Group *new_group = malloc(sizeof(Group));
    new_group->table = create_table(CAPACITY);
    new_group->group_id = malloc(MAX_LENGTH * sizeof(char));
    strcpy(new_group->group_id, group_id);
    new_group->apps_head = NULL;
    new_group->next = group_head;
    group_head = new_group;
}

void *com_thread(void *arg) {
    int n_bytes, k, flag = 0, i, app_sock, size;
    int *pos = (int *)arg;
    char *value;
    char *key = (char *)malloc(sizeof(char) * MAX_LENGTH);
    Group *group = group_head;
    time_t t;

    for (i = 0; i < *pos; i++) {
        group = group->next;
    }
    App *app = group->apps_head;
    app_sock = app->app_sock;

    while (1) {
        n_bytes = recv(app_sock, &flag, sizeof(int), 0);
        if (n_bytes == 0) {
            flag = 3;
        }
        // printf("oi\n");
        // printf("flag: %d\n", flag);
        switch (flag) {
            case 0:
                n_bytes = recv(app_sock, key, MAX_LENGTH, 0);
                n_bytes = recv(app_sock, &size, sizeof(int), 0);
                value = realloc(value, size * sizeof(char));
                n_bytes = recv(app_sock, value, size * sizeof(char), 0);
                ht_insert(group->table, key, value);
                // printf("%s-%s inserted\n", key, value);
                break;
            case 1:
                n_bytes = recv(app_sock, key, MAX_LENGTH, 0);
                // printf("vai entrar no search\n");
                value = ht_search(group->table, key);
                if (value != NULL) {
                    flag = 1;
                    size = strlen(value) + 1;
                    send(app_sock, &flag, sizeof(int), 0);
                    send(app_sock, &size, sizeof(int), 0);
                    send(app_sock, value, size, 0);
                } else {
                    flag = -1;
                    send(app_sock, &flag, sizeof(int), 0);
                }
                break;
            case 2:
                n_bytes = recv(app_sock, key, MAX_LENGTH, 0);
                if (delete_item(group->table, key) != NULL) {
                    flag = 1;
                    send(app_sock, &flag, sizeof(int), 0);
                } else {
                    flag = -1;
                    send(app_sock, &flag, sizeof(int), 0);
                }
                break;
            case 3:
                app->conected = 0;
                time(&t);
                app->t[1] = t;
                close(app_sock);
                pthread_exit(NULL);
                break;
        }
    }
}

void *accept_thread(void *arg) {
    int *server_sock = (int *)arg;
    int n_bytes, i = 0, aux_pos, flag = 0;
    pthread_t thread_com[10];
    int app_sock;
    char *group_id = malloc(MAX_LENGTH * sizeof(char));
    char *secret = malloc(MAX_LENGTH * sizeof(char));
    char confirmation[MAX_LENGTH];
    struct sockaddr_in auth_server_addr;
    struct sockaddr_un app_addr;
    int app_addr_size = sizeof(app_addr);
    int auth_addr_size = sizeof(auth_server_addr);
    auth_server_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &auth_server_addr.sin_addr);
    auth_server_addr.sin_port = htons(8080);
    Group *aux_group;
    time_t t;
    char *msg = malloc(MAX_LENGTH * sizeof(char));

    while (1) {
        app_sock = accept(server_sock[0], (struct sockaddr *)&app_addr,
                          &app_addr_size);

        n_bytes = recv(app_sock, group_id, sizeof(group_id), 0);
        n_bytes = recv(app_sock, secret, sizeof(group_id), 0);
        // printf("received: %s and %s\n", group_id, secret);
        sprintf(msg, "0%s%s%s%s", "\0", group_id, "\0", secret);
        sendto(server_sock[1], msg, MAX_LENGTH, MSG_CONFIRM,
               (const struct sockaddr *)&auth_server_addr, auth_addr_size);
        n_bytes =
            recvfrom(server_sock[1], &flag, sizeof(flag), 0,
                     (struct sockaddr *)&auth_server_addr, &auth_addr_size);
        send(app_sock, &flag, sizeof(int), 0);
        if (flag != 1) {
            continue;
        }
        // n_bytes = recv(clients[i], group_id, sizeof(group_id), 0);
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
        // printf("pos:%d\n", pos);
        App *new_app = malloc(sizeof(App));
        new_app->app_sock = app_sock;
        new_app->conected = 1;
        sscanf(app_addr.sun_path, "/tmp/app_socket_%d", &new_app->pid);
        new_app->next = aux_group->apps_head;
        aux_group->apps_head = new_app;
        aux_pos = pos;
        time(&t);
        new_app->t[0] = t;
        pthread_create(&thread_com[i], NULL, com_thread, &aux_pos);
        i++;
        pos = 0;
    }
}

int main() {
    char *command = (char *)malloc(MAX_LENGTH * sizeof(char));
    char *secret = (char *)malloc(MAX_LENGTH * sizeof(char));
    char *msg = (char *)malloc(MAX_LENGTH * sizeof(char));
    int server_sock[2], i = 0, flag;
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
        printf(
            "s \'group_id\' -> gives status of "
            "apps\n--------------------------\n> ");
        fgets(command, MAX_LENGTH, stdin);
        char *token = strtok(command, " ");
        if (strcmp(token, "c") == 0) {
            token = strtok(NULL, " ");
            token[strcspn(token, "\n")] = 0;
            sprintf(msg, "1%c%s%c", '\0', token, '\0');
            sendto(server_sock[1], msg, MAX_LENGTH, MSG_CONFIRM,
                   (const struct sockaddr *)&auth_server_addr, auth_addr_size);
            n_bytes =
                recvfrom(server_sock[1], secret, MAX_LENGTH, MSG_WAITALL,
                         (struct sockaddr *)&auth_server_addr, &auth_addr_size);
            if (secret != NULL) {
                create_group(token);
                printf("\nCreated group: %s\n", token);
                printf("Group secret: %s\n-----------------------\n", secret);
            } else {
                printf("\nGroup %s already exists\n----------------------\n",
                       token);
            }
        } else if (strcmp(token, "d") == 0) {
            token = strtok(NULL, " ");
            token[strcspn(token, "\n")] = 0;
            sprintf(msg, "2%s%s", "\0", token);
            sendto(server_sock[1], msg, MAX_LENGTH, MSG_CONFIRM,
                   (const struct sockaddr *)&auth_server_addr, auth_addr_size);
            n_bytes =
                recvfrom(server_sock[1], msg, MAX_LENGTH, MSG_WAITALL,
                         (struct sockaddr *)&auth_server_addr, &auth_addr_size);

        } else if (strcmp(token, "i") == 0) {
            // printf("alo?\n");
            flag = 3;
            token = strtok(NULL, " ");
            token[strcspn(token, "\n")] = 0;
            // printf("token: %s\n", token);
            sendto(server_sock[1], &flag, sizeof(flag), MSG_CONFIRM,
                   (const struct sockaddr *)&auth_server_addr, auth_addr_size);
            sendto(server_sock[1], token, sizeof(token), MSG_CONFIRM,
                   (const struct sockaddr *)&auth_server_addr, auth_addr_size);
            n_bytes =
                recvfrom(server_sock[1], secret, sizeof(secret), MSG_WAITALL,
                         (struct sockaddr *)&auth_server_addr, &auth_addr_size);
            Group *group = group_head;
            while (strcmp(group->group_id, token) != 0) {
                group = group->next;
            }

            printf("-----------------\nGroup %s info\n", token);
            printf("Secret: %s\n", secret);
            printf("Number of key-value pairs: %d\n------------------------\n",
                   group->table->count);
        } else if (strcmp(token, "s") == 0) {
            Group *group = group_head;
            token = strtok(NULL, " ");
            token[strcspn(token, "\n")] = 0;
            while (strcmp(group->group_id, token) != 0) {
                group = group->next;
            }
            App *app = group->apps_head;
            printf("----------------\nList of apps\n");
            while (app != NULL) {
                printf("Process %d:\n", app->pid);

                printf("  -> Conected on %s", ctime(&app->t[0]));
                if (app->conected == 0) {
                    printf("  -> Disconected on %s", ctime(&app->t[1]));
                }

                app = app->next;
            }
            printf("-------------------\n");
        } else {
            printf("\nInvalid command\n");
        }
    }
    remove(LOCAL_SERVER_ADDR);

    return 0;
}
