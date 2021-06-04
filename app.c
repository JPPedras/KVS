#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "lib.h"

#define SERVER_ADDR "/tmp/server_address"
#define MAX_LENGTH 512

void f1(char *changed_key) {
    printf("The key with name \'%s\' was changed\n", changed_key);
}

int main() {
    int flag = establish_connection("111", "password");
    // printf("flag: %d\n", flag);
    char *value;
    char *key = malloc(MAX_LENGTH * sizeof(char));

    /*flag = put_value("nome", "goncalo");
    flag = register_callback("nome", f1);
    flag = get_value("nome", &value);
    printf("nome: %s\n", value);
    flag = delete_value("nome");
    flag = put_value("armagedon", "pedrassdfsfsfs");
    flag = get_value("armagedon", &value);
    printf("armagedon: %s\n", value);
    flag = put_value("hellohello", "coelho");
    flag = get_value("hellohello", &value);
    printf("hellohello: %s\n", value);
    flag = put_value("nome", "andre");
    flag = get_value("nome", &value);
    printf("nome: %s\n", value);*/
    for (int i = 0; i < 100; i++) {
        sprintf(key, "%d", i);
        flag = put_value(key, "teste");
        if (flag == -1) {
            printf("Group was deleted\n");
            return 0;
        }
        usleep(400000);
    }

    // sleep(10);
    flag = close_connection();
    getchar();
    return 0;
}