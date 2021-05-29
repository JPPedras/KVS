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
    char *value;

    flag = put_value("nome", "goncalo");
    // flag = register_callback("nome", f1);
    flag = get_value("nome", &value);
    printf("nome: %s\n", value);
    flag = delete_value("nome");
    flag = put_value("armagedon", "pedrassdfsfsfs");
    flag = get_value("armagedon", &value);
    printf("armagedon: %s\n", value);
    flag = put_value("hellohello", "coelho");
    flag = get_value("hellohello", &value);
    printf("hellohello: %s\n", value);

    // sleep(10);
    // lag = close_connection();
    getchar();
    return 0;
}