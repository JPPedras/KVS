#include "hash_tables.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

void add_new_pair(Group* group, char* key, char* value) {
    int s;
    Pair* pair = malloc(sizeof(Pair));
    pthread_rwlock_init(&pair->rwlock, NULL);
    pair->key = malloc((strlen(key) + 1) * sizeof(char));
    pair->value = malloc((strlen(value) + 1) * sizeof(char));
    strcpy(pair->key, key);
    strcpy(pair->value, value);
    pair->count = 0;
    pair->mon = NULL;
    pair->next = group->pairs_head;
    group->pairs_head = pair;
}

void insert_pair(Group* group, char* key, char* value) {
    // create a link
    App* app;
    int flag = 1;
    Pair* pair = pair_search(group, key);

    if (pair == NULL) {
        add_new_pair(group, key, value);
    } else {
        pair->value = realloc(pair->value, (strlen(value) + 1) * sizeof(char));
        strcpy(pair->value, value);
        for (int i = 0; i < pair->count; i++) {
            app = group->apps_head;
            while (app != NULL) {
                if (app->pid == pair->mon[i] && app->conected == 1) {
                    send(app->app_sock[1], key, MAX_LENGTH, 0);
                    break;
                }
                app = app->next;
            }
        }
    }
}

int get_list_size(Group* group) {
    int length = 0;
    Pair* pair;

    for (pair = group->pairs_head; pair != NULL; pair = pair->next) {
        length++;
    }

    return length;
}

void free_pair(Pair* pair) {
    free(pair->key);
    free(pair->value);
    free(pair->mon);
    free(pair);
}
// find a link with given key
Pair* pair_search(Group* group, char* key) {
    // start from the first link
    int s;

    Pair* pair = group->pairs_head;

    Pair* aux_pair;

    if (pair == NULL) {
        return NULL;
    }

    // navigate through list
    while (strcmp(pair->key, key) != 0) {
        // if it is last node
        if (pair->next == NULL) {
            return NULL;
        } else {
            aux_pair = pair;
            pair = pair->next;
        }
    }
    // if data found, return the current Link
    return pair;
}
// delete a link with given key
int delete_pair(Group* group, char* key) {
    // start from the first link
    Pair* pair = group->pairs_head;
    Pair* aux_pair = NULL;

    if (pair == NULL) {
        return -1;
    }

    // navigate through list
    while (strcmp(pair->key, key) != 0) {
        // if it is last node
        if (pair->next == NULL) {
            return -1;
        } else {
            aux_pair = pair;
            pair = pair->next;
        }
    }

    // found a match, update the link
    if (pair == group->pairs_head) {
        group->pairs_head = pair->next;
    } else {
        aux_pair->next = pair->next;
    }
    free_pair(pair);

    return 1;
}

void add_monitor(Group* group, char* key, int pid) {
    // Searches the key in the hashtable
    // and returns NULL if it doesn't exist
    Pair* pair = pair_search(group, key);

    // Ensure that we move to a non NULL item
    if (pair != NULL) {
        pair->count++;
        pair->mon = realloc(pair->mon, pair->count * sizeof(int));
        pair->mon[pair->count - 1] = pid;
    }
}
