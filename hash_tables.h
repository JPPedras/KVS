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
    int app_sock;
    int pid;
    time_t t[2];
    int conected;
    struct App* next;
} App;

typedef struct Ht_item {
    char* key;
    char* value;
} Ht_item;

typedef struct Table {
    Ht_item** items;
    int size;
    int count;
} Table;

typedef struct Group {
    char* group_id;
    struct Table* table;
    struct App* apps_head;
    struct Group* next;
} Group;

typedef struct Msg {
    char* string1;
    char* string2;
    int flag;
} Msg;

unsigned long hash_function(char* key);
Ht_item* create_item(char* key, char* value);
Table* create_table(int size);
Msg* create_msg(char* string1, char* string2, int flag);
void free_item(Ht_item* item);
void free_table(Table* table);
void handle_collision(Table* table, unsigned long index, Ht_item* item);
void ht_insert(Table* table, char* key, char* value);
char* ht_search(Table* table, char* key);
char* delete_item(Table* table, char* key);