#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

int local_servers[10];
#define MAX_LENGTH 20

int main() {
    int auth_server_sock, i = 0, n_bytes;
    struct sockaddr_in auth_server_addr;
    struct sockaddr_in local_server_addr;
    int local_addr_size = sizeof(local_server_addr);
    int auth_addr_size = sizeof(auth_server_addr);
    auth_server_addr.sin_family = AF_INET;
    auth_server_addr.sin_port = htons(8080);
    // inet_aton("127.0.0.1", &auth_server_addr.sin_addr);
    auth_server_addr.sin_addr.s_addr = INADDR_ANY;
    // printf("%s\n", inet_addr(INADDR_ANY));
    char group_id[MAX_LENGTH];
    char secret[MAX_LENGTH];
    char confirm[MAX_LENGTH];

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

    while (1) {
        printf("vou receber:\n");
        n_bytes =
            recvfrom(auth_server_sock, group_id, sizeof(group_id), MSG_WAITALL,
                     (struct sockaddr *)&local_server_addr, &local_addr_size);
        printf("processing group_id: %s\n", group_id);
        strcpy(confirm, "Authorized");
        sendto(auth_server_sock, confirm, sizeof(confirm), MSG_CONFIRM,
               (struct sockaddr *)&local_server_addr, local_addr_size);
        printf("received: %s\n", group_id);
    }
    return 0;
}