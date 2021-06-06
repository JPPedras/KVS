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

/*
app_sock[0] -> socket for normal communication
app_sock[1] -> socket for callback communication
t[0] -> conection time
t[1] -> disconection time
conected -> 1:app is conected 0:app is disconected 
*/
typedef struct App {
    int app_sock[2];
    int pid;
    time_t t[2];
    int conected;
    struct App* next;
} App;

/*
mon -> array with the process ids of the apps monitoring this pair
count -> number of apps monitoring this pair 
*/
typedef struct Pair {
    char* key;
    char* value;
    int* mon;
    int count;
    struct Pair* next;
} Pair;

/*
active -> 1:group ok 0:group deleted
pairs_head -> list head for the list of pairs
apps_head -> list head for the list of apps  
*/
typedef struct Group {
    char* group_id;
    int active;
    pthread_rwlock_t rwlock;
    struct Pair* pairs_head;
    struct App* apps_head;
    struct Group* next;
} Group;

/*adds a new element to a linked list -> group_id/secret pair  OR  key/value pair*/
int add_new_pair(Group* group, char* key, char* value);

/*inserts or modifies a pair on a linked list*/
int insert_pair(Group* group, char* key, char* value);

/*returns the size of a linked list*/
int get_list_size(Group* group);

/*frees the memory allocated for a group_id/secret pair OR key/value pair*/
void free_pair(Pair* pair);

/*returns a pointer to a pair stored on a linked list */
Pair* pair_search(Group* group, char* key);

/*deletes a pair from a linked list*/
int delete_pair(Group* group, char* key);

/*adds the pid of an app to the list of monitoring apps of a certain pair*/
int add_monitor(Group* group, char* key, int pid);
