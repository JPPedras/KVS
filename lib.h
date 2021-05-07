int establish_connection(char* group_id, char* secret);
int put_value(char* key, char* value);
int get_value(char* key, char** value);
int delete_value(char* key);
int close_connection();