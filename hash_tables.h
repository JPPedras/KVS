#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#define CAPACITY 50000
#define MAX_LENGTH 512

typedef struct App {
    int app_sock[2];
    int pid;
    time_t t[2];
    int conected;
    struct App* next;
} App;

typedef struct Pair {
    char* key;
    char* value;
    int* mon;
    int count;
    struct Pair* next;
} Pair;

typedef struct Group {
    char* group_id;
    int active;
    pthread_rwlock_t rwlock;
    struct Pair* pairs_head;
    struct App* apps_head;
    struct Group* next;
} Group;

int add_new_pair(Group* group, char* key, char* value);
int insert_pair(Group* group, char* key, char* value);
int get_list_size(Group* group);
void free_pair(Pair* pair);
Pair* pair_search(Group* group, char* key);
int delete_pair(Group* group, char* key);
int add_monitor(Group* group, char* key, int pid);
