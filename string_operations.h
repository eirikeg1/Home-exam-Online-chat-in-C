

// returns 1 if valid username and 0 if not. Sets appropriate null byte if username is too long
int check_username(char *username);

// gets string from keyboard, removes newline at the end and all trailing chars after BUFSIZE
void get_string(char buf[], int size);

// Checks if string is number. Returns 1 if yes and 0 if no
int is_number(char *s);

// Splits string into double-pointer array on white-space. Mallocs a dynamic size
int *split(char ***destination, char *message);

// Copies everything after keyword 'MSG' in 'char *raw_message' to destination and returns index. Saves both to the heap
void get_msg(char **destination, char *raw_message);