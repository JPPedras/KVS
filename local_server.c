#include "hash_tables.h"
#include "lib.h"

#define LOCAL_SERVER_ADDR "/tmp/local_server_address"
#define LOCAL_SERVER_ADDR_CB "/tmp/local_server_address_cb"
#define MAX_LENGTH 512
Group *group_head = NULL;

void create_group(char *group_id) {
    Group *new_group = malloc(sizeof(Group));
    new_group->table = create_table(CAPACITY);
    new_group->group_id = malloc(MAX_LENGTH * sizeof(char));
    strcpy(new_group->group_id, group_id);
    new_group->active = 1;
    new_group->apps_head = NULL;
    new_group->next = group_head;
    group_head = new_group;
    pthread_rwlock_init(&new_group->rwlock, NULL);
}

Group *find_group(char *group_id) {
    Group *group = group_head;
    int s;

    while (group != NULL) {
        s = pthread_rwlock_rdlock(&group->rwlock);
        if (group->active == 1) {
            if (strcmp(group->group_id, group_id) == 0) {
                s = pthread_rwlock_unlock(&group->rwlock);
                break;
            }
        }
        s = pthread_rwlock_unlock(&group->rwlock);
        group = group->next;
    }

    return group;
}

void delete_group(char *group_id) {
    App *app, *aux_app;
    Group *group = find_group(group_id);
    free_table(group->table);
    group->table = NULL;
    free(group->group_id);
    group->group_id = NULL;
    group->active = 0;
}

void *com_thread(void *arg) {
    int n_bytes, k, flag = 0, flag2, i, app_sock[2], size, index2, s;
    int *pos = (int *)arg;
    char *value;
    char *key = (char *)malloc(sizeof(char) * MAX_LENGTH);
    Group *group = (Group *)arg;
    // printf("group_id: %s\n", group->group_id);
    time_t t;
    pthread_t thread_mon;

    App *app = group->apps_head;
    // app_sock[0] = app->app_sock[0];
    // app_sock[1] = app->app_sock[1];

    while (1) {
        n_bytes = recv(app->app_sock[0], &flag, sizeof(int), 0);
        if (n_bytes == 0) {
            flag = 3;
        } else if (group->active == 0) {
            flag = -2;
            send(app->app_sock[0], &flag, sizeof(int), 0);
            close(app->app_sock[0]);
            close(app->app_sock[1]);
            free(app);
            pthread_exit(NULL);
        } else if (n_bytes != 0) {
            flag2 = 1;
            send(app->app_sock[0], &flag2, sizeof(int), 0);
        }
        // printf("oi\n");
        // printf("flag: %d\n", flag);
        switch (flag) {
            // put_value
            case 0:
                n_bytes = recv(app->app_sock[0], key, MAX_LENGTH, 0);
                n_bytes = recv(app->app_sock[0], &size, sizeof(int), 0);
                value = realloc(value, size * sizeof(char));
                n_bytes = recv(app->app_sock[0], value, size * sizeof(char), 0);
                // printf("inserting pair: %s-%s\n", key, value);
                s = pthread_rwlock_wrlock(&group->rwlock);
                ht_insert(group, key, value);
                s = pthread_rwlock_unlock(&group->rwlock);
                // printf("%s-%s inserted\n", key, value);
                break;
            // get_value
            case 1:
                n_bytes = recv(app->app_sock[0], key, MAX_LENGTH, 0);
                // printf("vai entrar no search\n");
                s = pthread_rwlock_rdlock(&group->rwlock);
                if (ht_search(group->table, key) != NULL) {
                    value = realloc(value,
                                    (strlen(ht_search(group->table, key)) + 1) *
                                        sizeof(char));
                    value = ht_search(group->table, key);
                    s = pthread_rwlock_unlock(&group->rwlock);
                    flag = 1;
                    size = (strlen(value) + 1);
                    send(app->app_sock[0], &flag, sizeof(int), 0);
                    send(app->app_sock[0], &size, sizeof(int), 0);
                    send(app->app_sock[0], value, size, 0);
                } else {
                    s = pthread_rwlock_unlock(&group->rwlock);
                    flag = -1;
                    send(app->app_sock[0], &flag, sizeof(int), 0);
                }
                break;
            // delete_value
            case 2:
                n_bytes = recv(app->app_sock[0], key, MAX_LENGTH, 0);
                s = pthread_rwlock_wrlock(&group->rwlock);
                if (ht_search(group->table, key) != NULL) {
                    delete_item(group->table, key);
                    s = pthread_rwlock_unlock(&group->rwlock);
                    flag = 1;
                    send(app->app_sock[0], &flag, sizeof(int), 0);
                } else {
                    s = pthread_rwlock_unlock(&group->rwlock);
                    flag = -1;
                    send(app->app_sock[0], &flag, sizeof(int), 0);
                }
                break;
            // close_conect
            case 3:
                s = pthread_rwlock_wrlock(&group->rwlock);
                app->conected = 0;
                time(&t);
                app->t[1] = t;
                close(app->app_sock[0]);
                close(app->app_sock[1]);
                s = pthread_rwlock_unlock(&group->rwlock);
                pthread_exit(NULL);
                break;
            // register_callback
            case 4:
                n_bytes = recv(app->app_sock[0], key, MAX_LENGTH, 0);
                s = pthread_rwlock_wrlock(&group->rwlock);
                if (ht_search(group->table, key) != NULL) {
                    add_monitor(group->table, key, app->pid);
                    flag = 1;
                } else {
                    flag = -1;
                }
                s = pthread_rwlock_unlock(&group->rwlock);
                send(app->app_sock[0], &flag, sizeof(int), 0);
        }
    }
}

void *accept_thread(void *arg) {
    int *server_sock = (int *)arg;
    int n_bytes, i = 0, aux_pos, flag = 0, s;
    pthread_t thread_com;
    int app_sock[2];
    char *group_id = malloc(MAX_LENGTH * sizeof(char));
    char *secret = malloc(MAX_LENGTH * sizeof(char));
    char confirmation[MAX_LENGTH];
    struct sockaddr_in auth_server_addr;
    struct sockaddr_un app_addr[2];
    int app_addr_size = sizeof(app_addr[0]);
    int auth_addr_size = sizeof(auth_server_addr);
    auth_server_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &auth_server_addr.sin_addr);
    auth_server_addr.sin_port = htons(8080);
    Group *group;
    time_t t;
    char *msg = malloc(MAX_LENGTH * sizeof(char));

    while (1) {
        app_sock[0] = accept(server_sock[0], (struct sockaddr *)&app_addr[0],
                             &app_addr_size);

        n_bytes = recv(app_sock[0], group_id, MAX_LENGTH, 0);
        n_bytes = recv(app_sock[0], secret, MAX_LENGTH, 0);
        // printf("received: %s and %s\n", group_id, secret);
        sprintf(msg, "0%c%s%c%s", '\0', group_id, '\0', secret);
        sendto(server_sock[2], msg, MAX_LENGTH, MSG_CONFIRM,
               (const struct sockaddr *)&auth_server_addr, auth_addr_size);
        n_bytes =
            recvfrom(server_sock[2], &flag, sizeof(flag), 0,
                     (struct sockaddr *)&auth_server_addr, &auth_addr_size);
        send(app_sock[0], &flag, sizeof(int), 0);
        if (flag != 1) {
            continue;
        }
        listen(server_sock[1], 2);
        app_sock[1] = accept(server_sock[1], (struct sockaddr *)&app_addr[1],
                             &app_addr_size);

        // n_bytes = recv(clients[i], group_id, sizeof(group_id), 0);
        // if authorized
        group = find_group(group_id);
        s = pthread_rwlock_wrlock(&group->rwlock);
        // printf("pos:%d\n", pos);
        App *new_app = malloc(sizeof(App));
        new_app->app_sock[0] = app_sock[0];
        new_app->app_sock[1] = app_sock[1];
        new_app->conected = 1;
        sscanf(app_addr[0].sun_path, "/tmp/app_socket_%d", &new_app->pid);
        new_app->next = group->apps_head;
        group->apps_head = new_app;
        time(&t);
        new_app->t[0] = t;
        s = pthread_rwlock_unlock(&group->rwlock);
        pthread_create(&thread_com, NULL, com_thread, group);
    }
}

int main() {
    char *command = (char *)malloc(MAX_LENGTH * sizeof(char));
    char *secret = (char *)malloc(MAX_LENGTH * sizeof(char));
    char *msg = (char *)malloc(MAX_LENGTH * sizeof(char));
    int server_sock[3], i = 0, flag;
    struct sockaddr_un un_server_sock_addr[2];
    struct sockaddr_in in_server_sock_addr;
    struct sockaddr_in auth_server_addr;
    int un_server_addr_size = sizeof(un_server_sock_addr[0]);
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
    server_sock[1] = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock[1] == -1) {
        printf("fail to create socket\n");
        exit(-1);
    }
    server_sock[2] = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sock[2] == -1) {
        printf("fail to create socket\n");
        exit(-1);
    }
    un_server_sock_addr[0].sun_family = AF_UNIX;
    strcpy(un_server_sock_addr[0].sun_path, LOCAL_SERVER_ADDR);
    un_server_sock_addr[1].sun_family = AF_UNIX;
    strcpy(un_server_sock_addr[1].sun_path, LOCAL_SERVER_ADDR_CB);

    unlink(LOCAL_SERVER_ADDR);
    if (bind(server_sock[0], (struct sockaddr *)&un_server_sock_addr[0],
             un_server_addr_size) == -1) {
        perror("erro1:");
        exit(-1);
    }
    unlink(LOCAL_SERVER_ADDR_CB);
    if (bind(server_sock[1], (struct sockaddr *)&un_server_sock_addr[1],
             un_server_addr_size) == -1) {
        perror("erro2:");
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
    char *aux;
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
            sprintf(msg, "1%c%s", '\0', token);
            sendto(server_sock[2], msg, MAX_LENGTH, MSG_CONFIRM,
                   (const struct sockaddr *)&auth_server_addr, auth_addr_size);
            n_bytes =
                recvfrom(server_sock[2], msg, MAX_LENGTH, MSG_WAITALL,
                         (struct sockaddr *)&auth_server_addr, &auth_addr_size);
            aux = msg;
            if (strcmp(aux, "1") == 0) {
                // printf("hey\n");
                create_group(token);
                aux = strchr(aux, '\0');
                aux++;
                printf("\nCreated group: %s\n", token);
                printf("Group secret: %s\n-----------------------\n", aux);
            } else {
                printf("\nGroup %s already exists\n----------------------\n",
                       token);
            }
        } else if (strcmp(token, "d") == 0) {
            token = strtok(NULL, " ");
            token[strcspn(token, "\n")] = 0;
            sprintf(msg, "2%c%s", '\0', token);
            sendto(server_sock[2], msg, MAX_LENGTH, MSG_CONFIRM,
                   (const struct sockaddr *)&auth_server_addr, auth_addr_size);
            n_bytes =
                recvfrom(server_sock[2], &flag, sizeof(int), MSG_WAITALL,
                         (struct sockaddr *)&auth_server_addr, &auth_addr_size);
            // printf("flag: %d\n", flag);
            if (flag == 1) {
                delete_group(token);
                printf("\nGroup %s deleted\n----------------------\n", token);
            } else {
                printf("\nGroup %s does not exist\n----------------------\n",
                       token);
            }

        } else if (strcmp(token, "i") == 0) {
            token = strtok(NULL, " ");
            token[strcspn(token, "\n")] = 0;
            sprintf(msg, "3%c%s", '\0', token);
            sendto(server_sock[2], msg, MAX_LENGTH, MSG_CONFIRM,
                   (const struct sockaddr *)&auth_server_addr, auth_addr_size);
            n_bytes =
                recvfrom(server_sock[2], msg, MAX_LENGTH, MSG_WAITALL,
                         (struct sockaddr *)&auth_server_addr, &auth_addr_size);
            aux = msg;
            if (strcmp(aux, "1") == 0) {
                aux = strchr(aux, '\0');
                aux++;
                Group *group = find_group(token);

                printf("-----------------\nGroup %s info\n", token);
                printf("Secret: %s\n", aux);
                printf(
                    "Number of key-value pairs: %d\n------------------------\n",
                    group->table->count);
            } else {
                printf("\nGroup %s does not exist\n----------------------\n",
                       token);
            }
        } else if (strcmp(token, "s") == 0) {
            token = strtok(NULL, " ");
            token[strcspn(token, "\n")] = 0;
            Group *group = find_group(token);

            if (group != NULL) {
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
                printf("\nGroup %s does not exist\n----------------------\n",
                       token);
            }
        } else {
            printf("\nInvalid command\n");
        }
    }
    remove(LOCAL_SERVER_ADDR);

    return 0;
}
