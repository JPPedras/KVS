#include "hash_tables.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

unsigned long hash_function(char* key) {
    unsigned long i = 0;
    for (int j = 0; key[j]; j++) {
        i += key[j];
    }
    // printf("key: %s -> %ld\n", key, i % CAPACITY);
    return i % CAPACITY;
}

Ht_item* create_item(char* key, char* value) {
    // Creates a pointer to a new hash table item
    Ht_item* item = malloc(sizeof(Ht_item));
    printf("value: %s\n", value);
    printf("value len1: %ld\n", strlen(value));
    item->value =
        (char*)realloc(item->value, (strlen(value) + 1) * sizeof(char));
    if (item->value == NULL) {
        printf("erro\n");
    }
    printf("value len2: %ld\n", strlen(value));

    item->key = (char*)realloc(item->key, (strlen(key) + 1) * sizeof(char));
    perror("erro:");
    // item->value=NULL;
    // printf("len value:%ld\n", strlen(value));

    printf("passou malloc2\n");
    strcpy(item->key, key);
    strcpy(item->value, value);
    item->count = 0;

    return item;
}

Table* create_table(int size) {
    // Creates a new Group
    Table* table = (Table*)malloc(sizeof(Table));
    table->size = size;
    table->count = 0;
    table->items = (Ht_item**)calloc(table->size, sizeof(Ht_item*));
    for (int i = 0; i < table->size; i++) table->items[i] = NULL;
    return table;
}

Msg* create_msg(char* key, int index1, int index2) {
    Msg* new_msg = malloc(sizeof(Msg));
    new_msg->key = malloc((strlen(key) + 1) * sizeof(char));
    strcpy(new_msg->key, key);
    new_msg->index[0] = index1;
    new_msg->index[1] = index2;
    return new_msg;
}

void free_item(Ht_item* item) {
    // Frees an item
    free(item->key);
    free(item->value);
    free(item);
}

void free_table(Table* table) {
    // Frees the table
    for (int i = 0; i < table->size; i++) {
        Ht_item* item = table->items[i];
        if (item != NULL) free_item(item);
        item = NULL;
    }

    free(table->items);
    free(table);
}

void handle_collision(Table* table, unsigned long index, Ht_item* item) {
    printf("collision?!\n");
}

void ht_insert(Group* group, char* key, char* value) {
    // Create the item
    // printf("hey23\n");
    Ht_item* item = create_item(key, value);
    App* app;
    int flag = 1;
    // Compute the index
    unsigned long index = hash_function(key);
    Ht_item* current_item = group->table->items[index];

    // printf("hey\n");
    if (current_item == NULL) {
        // printf("pair: %s-%s is new\n", key, value);
        // Key does not exist.
        if (group->table->count == group->table->size) {
            // Hash Table Full
            printf("Insert Error: Hash Table is full\n");
            // Remove the create item
            free_item(item);

            return;
        }

        // Insert directly
        group->table->items[index] = item;
        group->table->count++;

    } else {
        // Scenario 1: We only need to update value

        if (strcmp(current_item->key, key) == 0) {
            current_item->value = realloc(current_item->value,
                                          (strlen(value) + 1) * sizeof(char));
            strcpy(current_item->value, value);
            for (int i = 0; i < current_item->count; i++) {
                app = group->apps_head;
                while (app != NULL) {
                    if (app->pid == current_item->mon[i] &&
                        app->conected == 1) {
                        send(app->app_sock[1], &flag, sizeof(int), 0);
                        break;
                    }
                    app = app->next;
                }
            }

            return;
        }

        else {
            // Scenario 2: Collision
            handle_collision(group->table, index, item);

            return;
        }
    }
}

char* ht_search(Table* table, char* key) {
    // Searches the key in the hashtable
    // and returns NULL if it doesn't exist
    // printf("oops1\n");
    int index = hash_function(key);
    Ht_item* item = table->items[index];

    // Ensure that we move to a non NULL item
    if (item != NULL) {
        if (strcmp(item->key, key) == 0) {
            // printf("pair: %s-%s\n", item->key, item->value);
            return item->value;
        }
    }
    return NULL;
}

void add_monitor(Table* table, char* key, int pid) {
    // Searches the key in the hashtable
    // and returns NULL if it doesn't exist
    int index = hash_function(key);
    Ht_item* item = table->items[index];

    // Ensure that we move to a non NULL item
    if (item != NULL) {
        item->count++;
        item->mon = realloc(item->mon, item->count * sizeof(int));
        item->mon[item->count - 1] = pid;
    }
}

void delete_item(Table* table, char* key) {
    int index = hash_function(key);
    Ht_item* item = table->items[index];

    // Ensure that we move to a non NULL item

    if (item != NULL) {
        if (strcmp(item->key, key) == 0) {
            // table->items[index] = NULL;
            free_item(item);
            table->count--;

            return;
        }

    } else {
        return;
    }
}
