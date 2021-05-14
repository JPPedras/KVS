#include "hash_tables.h"

#define MAX_LENGTH 20

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

    Table *table = create_table(CAPACITY);
    int flag;

    while (1) {
        n_bytes =
            recvfrom(auth_server_sock, &flag, sizeof(flag), MSG_WAITALL,
                     (struct sockaddr *)&local_server_addr, &local_addr_size);
        n_bytes =
            recvfrom(auth_server_sock, group_id, sizeof(group_id), MSG_WAITALL,
                     (struct sockaddr *)&local_server_addr, &local_addr_size);
        switch (flag) {
            // connect
            case 0:
                n_bytes = recvfrom(
                    auth_server_sock, secret, sizeof(secret), MSG_WAITALL,
                    (struct sockaddr *)&local_server_addr, &local_addr_size);
                if (strcmp(secret, ht_search(table, group_id)) == 0) {
                    sendto(auth_server_sock, &flag, sizeof(flag), MSG_CONFIRM,
                           (struct sockaddr *)&local_server_addr,
                           local_addr_size);
                } else {
                    flag = 1;
                    sendto(auth_server_sock, &flag, sizeof(flag), MSG_CONFIRM,
                           (struct sockaddr *)&local_server_addr,
                           local_addr_size);
                }
                break;
            // create group
            case 1:
                // sprintf(secret, "%d", rand() % 100000);
                if (ht_search(table, group_id) == NULL) {
                    sprintf(secret, "%s", "password");
                    ht_insert(table, group_id, secret);
                    sendto(auth_server_sock, secret, sizeof(secret),
                           MSG_CONFIRM, (struct sockaddr *)&local_server_addr,
                           local_addr_size);
                } else {
                }

                break;
            // delete group
            case 2:
                if (ht_search(table, group_id) != NULL) {
                    delete_item(table, group_id);
                }
                break;
            // get secret
            case 3:
                // printf("entrou no search\n");
                if (ht_search(table, group_id) != NULL) {
                    secret = ht_search(table, group_id);
                }
                sendto(auth_server_sock, secret, sizeof(secret), MSG_CONFIRM,
                       (struct sockaddr *)&local_server_addr, local_addr_size);
                break;
        }
    }
    return 0;
}