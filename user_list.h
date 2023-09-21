#ifndef USER_LIST_H
#define USER_LIST_H

int counter;

// User struct
struct User;

/* Finds and returns first user with matching nick. If no such nick exists, returns NULL.
If user is not registered as cache, it only returns users which was updated the last 30 seconds */
struct User *lookup_user(struct User *head, char *nick);

// Removes the first user with a matching nick, if it exists.
void remove_user(struct User **head, char *nick);

/* Creates a new user in heap. Searches nick, if exists removes the old and adds user
 to start of list. If head is null it is set to new user */
struct User *add_user(struct User **head, char *nick, struct sockaddr_in ip);

// registers user as cache
void register_as_cache(struct User *user);

// Recursively frees all users. Should be used from head
void free_users(struct User *user);

#endif // USER_LIST_H