#ifndef MESSAGE_LIST_H
#define MESSAGE_LIST_H

struct User;

// Message struct
struct Message;

// Finds and returns first user with matching text. If no such text exists, returns NULL
void message_count_recursion(struct Message *msg, int number);

void print_message_count(struct Message **head);

// Finds and returns first user with matching text. If no such text exists, returns NULL
struct Message *lookup_message(struct Message **head, int number);

// Removes the first user with a matching number, if it exists.
void remove_message(struct Message **head, char *number);

/* Creates a new message in heap.*/
struct Message *add_message(struct Message **head, int fd, int number, int counter, char *full_text, char *metadata, char *destination, struct sockaddr_in *address);

// Removes all messages with matching number
void checkout_message(struct Message *head, char *number);

// Sends messages from buffer at right time
int check_messages(struct Message **head, struct User **cache_head, int global_counter, int timeout);

// Updates Message's status value
int set_status(struct Message *msg, int status);

// Recursively frees all users. Should be used from head
void free_messages(struct Message *msg);

#endif // MESSAGE_LIST_H