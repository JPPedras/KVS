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

    flag = put_value("45", "wewewe");
    flag = register_callback("45", f1);

    getchar();
    return 0;
}