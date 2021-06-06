#include "ds.h"

#define MAX_LENGTH 512

int main() {
    int auth_server_sock, i = 0, n_bytes;
    struct sockaddr_in auth_server_addr;
    struct sockaddr_in local_server_addr;
    int local_addr_size = sizeof(local_server_addr);
    int auth_addr_size = sizeof(auth_server_addr);
    auth_server_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &auth_server_addr.sin_addr);
    auth_server_addr.sin_port = htons(8080);
    time_t t;
    srand(time(&t));
    char *group_id = malloc(MAX_LENGTH * sizeof(char));
    if (group_id == NULL) {
        perror("Erro no malloc");
        exit(-1);
    }
    char *secret = malloc(MAX_LENGTH * sizeof(char));
    if (secret == NULL) {
        perror("Erro no malloc");
        exit(-1);
    }
    char *msg = malloc(MAX_LENGTH * sizeof(char));
    if (msg == NULL) {
        perror("Erro no malloc");
        exit(-1);
    }

    auth_server_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (auth_server_sock == -1) {
        perror("Erro a criar socket");
        exit(-1);
    }
    if (bind(auth_server_sock, (const struct sockaddr *)&auth_server_addr,
             auth_addr_size) == -1) {
        perror("Erro no bind");
        exit(-1);
    }
    Pair *pair;
    Group *group = malloc(sizeof(Group));
    if (group == NULL) {
        perror("Erro no malloc");
        exit(-1);
    }
    int flag;
    char *aux, *aux2;

    while (1) {
        n_bytes =
            recvfrom(auth_server_sock, msg, MAX_LENGTH, MSG_WAITALL,
                     (struct sockaddr *)&local_server_addr, &local_addr_size);
        if (n_bytes == -1) {
            perror("Erro no recvfrom");
            exit(-1);
        }
        aux = msg;
        flag = atoi(aux);

        switch (flag) {
            // connect
            case 0:
                aux = strchr(aux, '\0');
                aux++;
                aux2 = strchr(aux, '\0');
                aux2++;

                pair = pair_search(group, aux);
                if (pair != NULL) {
                    if (strcmp(pair->value, aux2) == 0) {
                        flag = 0;
                    } else {
                        flag = -2;
                    }
                } else {
                    flag = -1;
                }
                n_bytes = sendto(auth_server_sock, &flag, sizeof(int), 0,
                                 (struct sockaddr *)&local_server_addr,
                                 local_addr_size);
                if (n_bytes == -1) {
                    perror("Erro no sendto");
                    exit(-1);
                }
                break;
            // create group
            case 1:
                flag = 1;
                aux = strchr(aux, '\0');
                aux++;
                sprintf(secret, "%d", rand() % 100000);
                pair = pair_search(group, aux);
                if (pair == NULL) {
                    flag = insert_pair(group, aux, secret);
                    sprintf(msg, "%d%c%s", flag, '\0', secret);
                } else {
                    sprintf(msg, "1%c%s", '\0', pair->value);
                }
                n_bytes = sendto(auth_server_sock, msg, MAX_LENGTH, 0,
                                 (struct sockaddr *)&local_server_addr,
                                 local_addr_size);
                if (n_bytes == -1) {
                    perror("Erro no recvfrom");
                    exit(-1);
                }
                break;
            // delete group
            case 2:
                aux = strchr(aux, '\0');
                aux++;
                pair = pair_search(group, aux);
                if (pair == NULL) {
                    flag = -1;
                } else {
                    flag = delete_pair(group, aux);
                    flag = -1;
                }
                n_bytes = sendto(auth_server_sock, &flag, sizeof(int), 0,
                                 (struct sockaddr *)&local_server_addr,
                                 local_addr_size);
                if (n_bytes == -1) {
                    perror("Erro no recvfrom");
                    exit(-1);
                }
                break;
            // get secret
            case 3:
                aux = strchr(aux, '\0');
                aux++;

                pair = pair_search(group, aux);
                if (pair != NULL) {
                    strcpy(secret, pair->value);
                    sprintf(msg, "1%c%s", '\0', secret);
                } else {
                    strcpy(msg, "-1");
                }
                n_bytes = sendto(auth_server_sock, msg, MAX_LENGTH, 0,
                                 (struct sockaddr *)&local_server_addr,
                                 local_addr_size);
                if (n_bytes == -1) {
                    perror("Erro no recvfrom");
                    exit(-1);
                }
                break;
        }
    }
    return 0;
}