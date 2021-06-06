#include "ds.h"

int add_new_pair(Group* group, char* key, char* value) {
    Pair* pair = malloc(sizeof(Pair));
    if (pair == NULL) {
        perror("Erro no malloc");
        return -4;
    }
    pair->key = malloc((strlen(key) + 1) * sizeof(char));
    if (pair->key == NULL) {
        perror("Erro no malloc");
        return -4;
    }
    pair->value = malloc((strlen(value) + 1) * sizeof(char));
    if (pair->value == NULL) {
        perror("Erro no malloc");
        return -4;
    }
    strcpy(pair->key, key);
    strcpy(pair->value, value);
    pair->count = 0;
    pair->mon = NULL;
    pair->next = group->pairs_head;
    group->pairs_head = pair;

    return 1;
}

int insert_pair(Group* group, char* key, char* value) {
    
    App* app;
    int flag = 1, n_bytes;
    Pair* pair = pair_search(group, key);

    if (pair == NULL) {
        flag = add_new_pair(group, key, value);
    } else {
        pair->value = realloc(pair->value, (strlen(value) + 1) * sizeof(char));
        if (pair->value == NULL) {
            perror("Erro no malloc");
            return -4;
        }
        strcpy(pair->value, value);
        for (int i = 0; i < pair->count; i++) {
            app = group->apps_head;
            while (app != NULL) {
                if (app->pid == pair->mon[i] && app->conected == 1) {
                    n_bytes = send(app->app_sock[1], key, MAX_LENGTH, 0);
                    if (n_bytes == -1) {
                        perror("Erro no send");
                        exit(-1);
                    }
                    break;
                }
                app = app->next;
            }
        }
    }
    return flag;
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

Pair* pair_search(Group* group, char* key) {
    
    int s;

    Pair* pair = group->pairs_head;

    Pair* aux_pair;

    if (pair == NULL) {
        return NULL;
    }

   
    while (strcmp(pair->key, key) != 0) {
        
        if (pair->next == NULL) {
            return NULL;
        } else {
            aux_pair = pair;
            pair = pair->next;
        }
    }
   
    return pair;
}

int delete_pair(Group* group, char* key) {
    
    Pair* pair = group->pairs_head;
    Pair* aux_pair = NULL;

    if (pair == NULL) {
        return -2;
    }

    
    while (strcmp(pair->key, key) != 0) {
        
        if (pair->next == NULL) {
            return -2;
        } else {
            aux_pair = pair;
            pair = pair->next;
        }
    }

    
    if (pair == group->pairs_head) {
        group->pairs_head = pair->next;
    } else {
        aux_pair->next = pair->next;
    }
    free_pair(pair);

    return 1;
}

int add_monitor(Group* group, char* key, int pid) {
    Pair* pair = pair_search(group, key);
    int i;
    if (pair != NULL) {
        // check if pid already exists in the mon
        for (i = 0; i < pair->count; i++) {
            if (pid == pair->mon[i]) {
                return 1;
            }
        }
        pair->count++;
        pair->mon = realloc(pair->mon, pair->count * sizeof(int));
        if (pair->mon == NULL) {
            perror("Erro no malloc");
            return -4;
        }
        pair->mon[pair->count - 1] = pid;
        return 1;
    } else {
        return -2;
    }
}
