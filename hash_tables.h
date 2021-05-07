#define CAPACITY 50000
typedef struct Ht_item {
    char* key;
    char* value;
} Ht_item;

typedef struct Group {
    Ht_item** items;
    int size;
    int count;
} Group;

unsigned long hash_function(char* key);
Ht_item* create_item(char* key, char* value);
Group* create_table(int size);
void free_item(Ht_item* item);
void free_table(Group* table);
void handle_collision(Group* table, unsigned long index, Ht_item* item);
void ht_insert(Group* table, char* key, char* value);
char* ht_search(Group* table, char* key);
void delete_item(Group* table, char* key);