#include "hash_tables.h"

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
    // auth_server_addr.sin_addr.s_addr = INADDR_ANY;
    // printf("%s\n", inet_addr(INADDR_ANY));
    char *group_id = malloc(MAX_LENGTH * sizeof(char));
    char *secret = malloc(MAX_LENGTH * sizeof(char));
    char *msg = malloc(MAX_LENGTH * sizeof(char));

    auth_server_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (auth_server_sock == -1) {
        printf("fail to create socket\n");
        exit(-1);
    }
    if (bind(auth_server_sock, (const struct sockaddr *)&auth_server_addr,
             auth_addr_size) == -1) {
        perror("erro:");
        printf("error on bind\n");
        exit(-1);
    }

    Group *group = malloc(sizeof(Group));
    group->table = create_table(CAPACITY);
    int flag;
    char *aux, *aux2;

    while (1) {
        n_bytes =
            recvfrom(auth_server_sock, msg, MAX_LENGTH, MSG_WAITALL,
                     (struct sockaddr *)&local_server_addr, &local_addr_size);
        aux = msg;
        flag = atoi(aux);

        switch (flag) {
            // connect
            case 0:
                aux = strchr(aux, '\0');
                aux++;
                aux2 = strchr(aux, '\0');
                aux2++;
                // printf("received: %s and %s\n", aux, aux2);
                if (ht_search(group->table, aux) != NULL) {
                    if (strcmp(aux2, ht_search(group->table, aux)) == 0) {
                        flag = 1;
                    }
                }
                sendto(auth_server_sock, &flag, sizeof(int), MSG_CONFIRM,
                       (struct sockaddr *)&local_server_addr, local_addr_size);
                break;
            // create group
            case 1:
                aux = strchr(aux, '\0');
                aux++;
                // sprintf(secret, "%d", rand() % 100000);
                if (ht_search(group->table, aux) == NULL) {
                    sprintf(secret, "%s", "password");
                    // printf("secret: %s\n", secret);
                    ht_insert(group, aux, secret);
                    sprintf(msg, "1%c%s", '\0', "password");
                } else {
                    strcpy(msg, "0");
                }
                sendto(auth_server_sock, msg, MAX_LENGTH, MSG_CONFIRM,
                       (struct sockaddr *)&local_server_addr, local_addr_size);

                break;
            // delete group
            case 2:
                // printf(("hey\n"));
                aux = strchr(aux, '\0');
                aux++;
                if (ht_search(group->table, aux) != NULL) {
                    delete_item(group->table, aux);
                    flag = 1;
                } else {
                    // printf("hey\n");
                    flag = 0;
                }
                sendto(auth_server_sock, &flag, sizeof(int), MSG_CONFIRM,
                       (struct sockaddr *)&local_server_addr, local_addr_size);
                break;
            // get secret
            case 3:
                aux = strchr(aux, '\0');
                aux++;
                // printf("entrou no search\n");
                if (ht_search(group->table, aux) != NULL) {
                    secret = ht_search(group->table, aux);
                    sprintf(msg, "1%c%s", '\0', secret);
                } else {
                    strcpy(msg, "0");
                }
                sendto(auth_server_sock, msg, MAX_LENGTH, MSG_CONFIRM,
                       (struct sockaddr *)&local_server_addr, local_addr_size);
                break;
        }
    }
    return 0;
}