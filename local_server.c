#include "hash_tables.h"
#include "lib.h"

#define LOCAL_SERVER_ADDR "/tmp/local_server_address"
#define LOCAL_SERVER_ADDR_CB "/tmp/local_server_address_cb"
#define MAX_LENGTH 512
Group *group_head = NULL;
int server_sock[3];
struct sockaddr_un un_server_sock_addr[2];
struct sockaddr_in in_server_sock_addr;
struct sockaddr_in auth_server_addr;
int un_server_addr_size;
int in_server_addr_size;
int auth_addr_size;

/*Flags from auth_server
1 -> success
-1 -> group no longer available
-2 -> incorrect password
-3 -> memory error
*/
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

void create_group(char *group_id) {
    int n_bytes, i;
    char *secret = (char *)malloc(MAX_LENGTH * sizeof(char));
    char *msg = (char *)malloc(MAX_LENGTH * sizeof(char));
    char *aux;

    Group *group = find_group(group_id);
    if (group == NULL) {
        sprintf(msg, "1%c%s", '\0', group_id);
        for (i = 0; i < 20; i++) {
            sendto(server_sock[2], msg, MAX_LENGTH, 0,
                   (const struct sockaddr *)&auth_server_addr, auth_addr_size);
            n_bytes =
                recvfrom(server_sock[2], msg, MAX_LENGTH, MSG_DONTWAIT,
                         (struct sockaddr *)&auth_server_addr, &auth_addr_size);
            usleep(500000);
            if (n_bytes != 0) {
                break;
            }
        }
        if (i == 20) {
            printf("\nCreate timed out\n---------------------\n");
            return;
        }
        aux = msg;
        if (strcmp(aux, "1") == 0) {
            Group *new_group = malloc(sizeof(Group));
            new_group->group_id = malloc(MAX_LENGTH * sizeof(char));
            strcpy(new_group->group_id, group_id);
            new_group->active = 1;
            new_group->apps_head = NULL;
            new_group->pairs_head = NULL;
            new_group->next = group_head;
            group_head = new_group;
            pthread_rwlock_init(&new_group->rwlock, NULL);
            aux = strchr(aux, '\0');
            aux++;
            printf("\nCreated group: %s\n", group_id);
            printf("Group secret: %s\n-----------------------\n", aux);
        } else {
            printf(
                "\nErro no Authentication "
                "server\n-------------------------\n");
        }
    } else {
        printf("\nGroup %s already exists\n----------------------\n", group_id);
    }
}

void delete_group(char *group_id) {
    int i, n_bytes, flag, s;
    char *msg = (char *)malloc(MAX_LENGTH * sizeof(char));
    char *aux;

    Group *group = find_group(group_id);
    if (group != NULL) {
        sprintf(msg, "2%c%s", '\0', group_id);
        for (i = 0; i < 20; i++) {
            sendto(server_sock[2], msg, MAX_LENGTH, 0,
                   (const struct sockaddr *)&auth_server_addr, auth_addr_size);
            usleep(500000);
            n_bytes =
                recvfrom(server_sock[2], &flag, sizeof(int), MSG_DONTWAIT,
                         (struct sockaddr *)&auth_server_addr, &auth_addr_size);
            if (n_bytes != 0) {
                break;
            }
        }
        if (i == 20) {
            printf(
                "\nDelete on Auth_Server timed out\n-----------------------\n");
            return;
        }
        if (flag == 1 || flag == -1) {
            App *app;
            Pair *pair;
            s = pthread_rwlock_wrlock(&group->rwlock);
            free(group->group_id);
            group->group_id = NULL;
            group->active = 0;

            while (group->pairs_head != NULL) {
                pair = group->pairs_head;
                group->pairs_head = group->pairs_head->next;
                free_pair(pair);
            }
            s = pthread_rwlock_unlock(&group->rwlock);
            printf("\nGroup %s deleted\n----------------------\n", group_id);
        }
    } else {
        printf("\nGroup %s does not exist\n----------------------\n", group_id);
    }
}

void get_info(char *group_id) {
    int i, n_bytes, flag, s;
    char *msg = (char *)malloc(MAX_LENGTH * sizeof(char));
    char *aux;

    Group *group = find_group(group_id);
    if (group != NULL) {
        sprintf(msg, "3%c%s", '\0', group_id);
        for (i = 0; i < 20; i++) {
            sendto(server_sock[2], msg, MAX_LENGTH, 0,
                   (const struct sockaddr *)&auth_server_addr, auth_addr_size);
            usleep(500000);
            n_bytes =
                recvfrom(server_sock[2], msg, MAX_LENGTH, MSG_DONTWAIT,
                         (struct sockaddr *)&auth_server_addr, &auth_addr_size);
            if (n_bytes != 0) {
                break;
            }
        }
        if (i == 20) {
            printf(
                "\nGet info on Auth_Server timed "
                "out\n-----------------------\n");
            return;
        }
        aux = msg;
        if (strcmp(aux, "1") == 0) {
            aux = strchr(aux, '\0');
            aux++;

            printf("-----------------\nGroup %s info\n", group_id);
            printf("Secret: %s\n", aux);
            s = pthread_rwlock_rdlock(&group->rwlock);
            printf("Number of key-value pairs: %d\n------------------------\n",
                   get_list_size(group));
            s = pthread_rwlock_unlock(&group->rwlock);
        } else if (strcmp(aux, "-1") == 0) {
            printf("\nGroup no longer available in authentication server\n");
            printf("Group %s will be deleted\n", group_id);
            delete_group(group_id);
        }
    } else {
        printf("\nGroup %s does not exist\n----------------------\n", group_id);
    }
}

void get_status(char *group_id) {
    int i, n_bytes, flag, s;
    char *msg = (char *)malloc(MAX_LENGTH * sizeof(char));
    char *aux;

    Group *group = find_group(group_id);

    if (group != NULL) {
        s = pthread_rwlock_rdlock(&group->rwlock);
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
        s = pthread_rwlock_unlock(&group->rwlock);
    } else {
        printf("\nGroup %s does not exist\n----------------------\n", group_id);
    }
}

void close_app(App *app) {
    close(app->app_sock[0]);
    close(app->app_sock[1]);
    free(app);
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
    Pair *pair;
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
            s = pthread_rwlock_wrlock(&group->rwlock);
            close(app->app_sock[0]);
            close(app->app_sock[1]);
            free(app);
            s = pthread_rwlock_unlock(&group->rwlock);
            pthread_exit(NULL);
        } else if (n_bytes != 0) {
            flag2 = 1;
            send(app->app_sock[0], &flag2, sizeof(int), 0);
        }
        switch (flag) {
            // put_value
            case 0:
                n_bytes = recv(app->app_sock[0], key, MAX_LENGTH, 0);
                n_bytes = recv(app->app_sock[0], &size, sizeof(int), 0);
                value = realloc(value, size * sizeof(char));
                n_bytes = recv(app->app_sock[0], value, size * sizeof(char), 0);
                // printf("inserting pair: %s-%s\n", key, value);
                s = pthread_rwlock_wrlock(&group->rwlock);
                if (group->active == 1) {
                    insert_pair(group, key, value);
                }
                s = pthread_rwlock_unlock(&group->rwlock);
                // printf("%s-%s inserted\n", key, value);
                break;
            // get_value
            case 1:
                // printf("entrou\n");
                n_bytes = recv(app->app_sock[0], key, MAX_LENGTH, 0);

                s = pthread_rwlock_rdlock(&group->rwlock);
                if (group->active == 1) {
                    pair = pair_search(group, key);
                    if (pair != NULL) {
                        value = realloc(
                            value, (strlen(pair->value) + 1) * sizeof(char));
                        strcpy(value, pair->value);
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
                } else {
                    flag = -2;
                    s = pthread_rwlock_unlock(&group->rwlock);
                    send(app->app_sock[0], &flag, sizeof(int), 0);
                }
                break;
            // delete_value
            case 2:
                n_bytes = recv(app->app_sock[0], key, MAX_LENGTH, 0);
                s = pthread_rwlock_wrlock(&group->rwlock);
                if (group->active == 1) {
                    if (delete_pair(group, key) == 1) {
                        flag = 1;

                    } else {
                        flag = -1;
                    }
                } else {
                    flag = -2;
                }
                s = pthread_rwlock_unlock(&group->rwlock);
                send(app->app_sock[0], &flag, sizeof(int), 0);
                break;
            // close_conect
            case 3:
                s = pthread_rwlock_wrlock(&group->rwlock);
                if (group->active == 1) {
                    app->conected = 0;
                    time(&t);
                    app->t[1] = t;
                }
                close(app->app_sock[0]);
                close(app->app_sock[1]);
                s = pthread_rwlock_unlock(&group->rwlock);
                pthread_exit(NULL);
                break;
            // register_callback
            case 4:
                n_bytes = recv(app->app_sock[0], key, MAX_LENGTH, 0);
                s = pthread_rwlock_wrlock(&group->rwlock);
                if (group->active == 1) {
                    if (pair_search(group, key) != NULL) {
                        add_monitor(group, key, app->pid);
                        flag = 1;
                    } else {
                        flag = -1;
                    }
                } else {
                    flag = -2;
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
        if (n_bytes == 0) {
            continue;
        }
        n_bytes = recv(app_sock[0], secret, MAX_LENGTH, 0);
        if (n_bytes == 0) {
            continue;
        }
        group = find_group(group_id);
        if (group != NULL) {
            sprintf(msg, "0%c%s%c%s", '\0', group_id, '\0', secret);
            for (i = 0; i < 20; i++) {
                sendto(server_sock[2], msg, MAX_LENGTH, 0,
                       (const struct sockaddr *)&auth_server_addr,
                       auth_addr_size);
                usleep(500000);
                n_bytes = recvfrom(
                    server_sock[2], &flag, sizeof(flag), MSG_DONTWAIT,
                    (struct sockaddr *)&auth_server_addr, &auth_addr_size);
                if (n_bytes != 0) {
                    break;
                }
            }
            if (i == 20) {
                flag = -3;
            }
            if (flag == -1) {
                printf(
                    "\nGroup no longer available in authentication "
                    "server\nGroup "
                    "%s will be deleted",
                    group_id);
                delete_group(group_id);
            }
        } else {
            flag = -1;
        }
        send(app_sock[0], &flag, sizeof(int), 0);
        if (flag != 1) {
            continue;
        }
        listen(server_sock[1], 2);
        app_sock[1] = accept(server_sock[1], (struct sockaddr *)&app_addr[1],
                             &app_addr_size);

        // n_bytes = recv(clients[i], group_id, sizeof(group_id), 0);
        // if authorized
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
    int i = 0, flag, s;
    un_server_addr_size = sizeof(un_server_sock_addr[0]);
    in_server_addr_size = sizeof(in_server_sock_addr);
    auth_addr_size = sizeof(auth_server_addr);
    pthread_t ac_thread;
    int n_bytes;
    auth_server_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &auth_server_addr.sin_addr);
    auth_server_addr.sin_port = htons(8080);
    Group *group;

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
            create_group(token);

        } else if (strcmp(token, "d") == 0) {
            token = strtok(NULL, " ");
            token[strcspn(token, "\n")] = 0;
            delete_group(token);

        } else if (strcmp(token, "i") == 0) {
            token = strtok(NULL, " ");
            token[strcspn(token, "\n")] = 0;
            get_info(token);

        } else if (strcmp(token, "s") == 0) {
            token = strtok(NULL, " ");
            token[strcspn(token, "\n")] = 0;
            get_status(token);

        } else {
            printf("\nInvalid command\n");
        }
    }
    remove(LOCAL_SERVER_ADDR);

    return 0;
}
