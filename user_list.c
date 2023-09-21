#include "header.h"

#include "user_list.h"

// User struct
struct User
{
    int counter;
    int cache;
    int failcount; // Used only for cache. Keeps track of how many lookup calls has failed
    char *nick;
    struct sockaddr_in ip;
    struct User *next;
};

// Sets failcount to value
void set_failcount(struct User *user, int value)
{
    user->failcount = value;
}

// Returns users failcount
int get_failcount(struct User *user)
{
    return user->failcount;
}

/* Finds and returns first user with matching nick. If no such nick exists, returns NULL.
If user is not registered as cache, it only returns users which was updated the last 30 seconds */
struct User *lookup_user(struct User *head, char *nick)
{
    struct User *user = head;
    while (user != NULL)
    {
        if (strcmp(user->nick, nick) == 0)
        {
            if (((counter - user->counter) < 30 || user->cache == 1))
                return user;
        }

        user = user->next;
    }
    return NULL;
}

/* Finds and returns first user with matching nick. If no such nick exists, returns NULL.
Ignores counter */
struct User *lookup_blocked_user(struct User *head, char *nick)
{
    struct User *user = head;
    while (user != NULL)
    {
        if (strcmp(user->nick, nick) == 0)
        {
            return user;
        }

        user = user->next;
    }
    return NULL;
}

// Returns address
struct sockaddr_in *get_address(struct User *head, struct User *user)
{
    if (lookup_user(head, user->nick) == NULL)
        return NULL;
    return &(user->ip);
}

// Removes the first user with a matching nick, if it exists.
void remove_user(struct User **head, char *nick)
{
    struct User *user = *head;
    if (strcmp(user->nick, nick) == 0)
    {
        *head = user->next;
        free(user->nick);
        free(user);
        return;
    }
    while (user != NULL)
    {
        if (user->next == NULL)
        {
            return;
        }
        else if (strcmp(user->next->nick, nick) == 0)
        {
            struct User *tmp = user->next->next;
            free(user->next->nick);
            free(user->next);
            user->next = tmp;
            return;
        }

        user = user->next;
    }
    return;
}

/* Creates a new user in heap. Searches nick, if exists removes the old and adds user
 to start of list. If head is null it is set to new user */
struct User *add_user(struct User **head, char *nick, struct sockaddr_in ip)
{
    struct User *user = malloc(sizeof(struct User));
    if (user == NULL)
    {
        perror("malloc");
        close(fd);
        free(init_time);
        free(time2);
        free_users(user_head);
        free(raw_message);
        free(receive_addr);
        free(received_messages);
        free_messages(msg_head);
        exit(EXIT_FAILURE);
    }
    if (*head == NULL)
    {
        *head = user;
        user->next = NULL;
    }
    else
    {
        // checks if user already exists, if so deletes the old
        struct User *check = lookup_user(*head, nick);
        if (check != NULL)
        {
            remove_user(head, check->nick);
        }
        user->next = *head;
        *head = user;
    }
    user->cache = 0;
    user->counter = counter;
    user->failcount = 0;
    user->nick = strdup(nick);
    user->ip = ip;
    user->ip.sin_family = AF_INET;
    user->ip.sin_addr = ip.sin_addr;
    user->ip.sin_addr.s_addr = ip.sin_addr.s_addr;
    return user;
}

// registers user as cache
void register_as_cache(struct User *user)
{
    user->cache = 1;
}

// Recursively frees all users. Should be used from head
void free_users(struct User *user)
{
    if (user == NULL)
    {
        return;
    }
    if (user->next != NULL)
    {
        free_users(user->next);
    }
    free(user->nick);
    free(user);
}