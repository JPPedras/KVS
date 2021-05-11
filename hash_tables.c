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
    return i % CAPACITY;
}

Ht_item* create_item(char* key, char* value) {
    // Creates a pointer to a new hash table item
    Ht_item* item = (Ht_item*)malloc(sizeof(Ht_item));
    item->key = (char*)malloc(strlen(key) + 1);
    item->value = (char*)malloc(strlen(value) + 1);

    strcpy(item->key, key);
    strcpy(item->value, value);

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

Msg* create_msg(char* string1, char* string2, int flag) {
    Msg* new_msg = malloc(sizeof(Msg));
    new_msg->string1 = malloc(MAX_LENGTH * sizeof(char));
    new_msg->string2 = malloc(MAX_LENGTH * sizeof(char));
    strcpy(new_msg->string1, string1);
    if (string2 != NULL) {
        strcpy(new_msg->string2, string2);
    }
    new_msg->flag = flag;

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
    }

    free(table->items);
    free(table);
}

void handle_collision(Table* table, unsigned long index, Ht_item* item) {}

void ht_insert(Table* table, char* key, char* value) {
    // Create the item
    Ht_item* item = create_item(key, value);

    // Compute the index
    unsigned long index = hash_function(key);

    Ht_item* current_item = table->items[index];

    if (current_item == NULL) {
        // Key does not exist.
        if (table->count == table->size) {
            // Hash Table Full
            printf("Insert Error: Hash Table is full\n");
            // Remove the create item
            free_item(item);
            return;
        }

        // Insert directly
        table->items[index] = item;
        table->count++;
    }

    else {
        // Scenario 1: We only need to update value
        if (strcmp(current_item->key, key) == 0) {
            strcpy(table->items[index]->value, value);
            return;
        }

        else {
            // Scenario 2: Collision
            // We will handle case this a bit later
            handle_collision(table, index, item);
            return;
        }
    }
}

char* ht_search(Table* table, char* key) {
    // Searches the key in the hashtable
    // and returns NULL if it doesn't exist
    int index = hash_function(key);
    Ht_item* item = table->items[index];

    // Ensure that we move to a non NULL item
    if (item != NULL) {
        if (strcmp(item->key, key) == 0) return item->value;
    }
    return NULL;
}

void delete_item(Table* table, char* key) {
    int index = hash_function(key);
    Ht_item* item = table->items[index];

    // Ensure that we move to a non NULL item
    if (item != NULL) {
        if (strcmp(item->key, key) == 0) {
            free_item(item);
        }
    }
}
