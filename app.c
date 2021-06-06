#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "KVS-lib.h"

#define SERVER_ADDR "/tmp/server_address"
#define MAX_LENGTH 512

void f1(char *changed_key) {
    printf("F1: The key with name \'%s\' was changed\n", changed_key);
}

void f2(char *changed_key) {
    printf("F2: The key with name \'%s\' was changed\n", changed_key);
}

int main() {
    char *group_id = malloc(MAX_LENGTH * sizeof(char));
    char *secret = malloc(MAX_LENGTH * sizeof(char));
    printf("group_id > ");
    fgets(group_id, MAX_LENGTH, stdin);
    printf("secret > ");
    fgets(secret, MAX_LENGTH, stdin);
    group_id[strcspn(group_id, "\n")] = 0;
    secret[strcspn(secret, "\n")] = 0;
    int flag = establish_connection(group_id, secret);
    printf("flag: %d\n", flag);
    char *value;
    char *key = malloc(MAX_LENGTH * sizeof(char));

    for (int i = 0; i < 200; i++) {
        sprintf(key, "%d", i);
        flag = put_value(key, "lol");
        // printf("put_value %s -> flag: %d\n", key, flag);
        // usleep(50000);
    }
    flag = register_callback("3", f1);
    flag = register_callback("4", f2);
    flag = register_callback("6", f1);
    for (int i = 0; i < 400; i++) {
        sprintf(key, "%d", i);
        flag = get_value(key, &value);
        printf("get_value %s -> flag: %d\n", value, flag);
        usleep(50000);
    }

    // flag = close_connection();

    getchar();
    return 0;
}