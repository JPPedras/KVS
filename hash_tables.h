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

typedef struct Ht_item {
    int* mon;
    int count;
    char* key;
    char* value;
} Ht_item;

typedef struct Msg {
    char* key;
    int index[2];
} Msg;

typedef struct Table {
    Ht_item** items;
    int size;
    int count;
} Table;

typedef struct Group {
    char* group_id;
    int active;
    pthread_rwlock_t rwlock;
    struct Table* table;
    struct App* apps_head;
    struct Group* next;
} Group;

unsigned long hash_function(char* key);
Ht_item* create_item(char* key, char* val);
Table* create_table(int size);
void free_item(Ht_item* item);
void free_table(Table* table);
void handle_collision(Table* table, unsigned long index, Ht_item* item);
void ht_insert(Group* group, char* key, char* value);
char* ht_search(Table* table, char* key);
void delete_item(Table* table, char* key);
Msg* create_msg(char* key, int index1, int index2);
void add_monitor(Table* table, char* key, int pid);