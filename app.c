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
    printf("flag: %d\n", flag);
    char *value = malloc(MAX_LENGTH * sizeof(char));

    flag = put_value("nome", "goncalo");
    // flag = register_callback("nome", f1);
    flag = get_value("nome", &value);
    printf("nome: %s\n", value);
    flag = delete_value("nome");
    printf("hey\n");
    flag = put_value("3", "pedras");
    flag = get_value("3", &value);
    printf("3: %s\n", value);
    flag = put_value("2", "coelho");
    flag = get_value("2", &value);
    printf("2: %s\n", value);

    // sleep(10);
    // lag = close_connection();
    getchar();
    return 0;
}