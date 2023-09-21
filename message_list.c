#include "common.h"
#include "send_packet.h"
#include "user_list.h"
#include "string_operations.h"

struct User;

// Message struct
struct Message
{
    int counter;
    int number;
    int fd;
    int status; // 1 if waiting to be sent, 0 if idle, -1 checks out all with same number and deletes user from cache, -2 means server was not found
    char *metadata;
    char *full_text;
    char *destination;
    struct sockaddr_in *address;
    struct Message *next;
};

// Prints amount of messages in message_list
void message_count_recursion(struct Message *msg, int number)
{
    if (msg != NULL)
    {
        message_count_recursion(msg->next, ++number);
    }
    else
        printf("Message list size: %i\n", number);
    return;
}

// Prints amount of messages in message_list
void print_message_count(struct Message **head)
{
    if (*head != NULL)
    {
        message_count_recursion(*head, 1);
    }
    return;
}

// Finds and returns first user with matching text. If no such text exists, returns NULL
struct Message *lookup_message(struct Message **head, int number)
{
    struct Message *tmp = *head;
    while (tmp != NULL)
    {
        if (tmp->number == number)
            return tmp;

        tmp = tmp->next;
    }
    return NULL;
}

// Count message number
int *count_message_number(struct Message **head, int number)
{
    int *counter = malloc(sizeof(int));
    *counter = 0;
    struct Message *msg = *head;
    while (msg != NULL)
    {
        if (msg->number == number)
            (*counter)++;

        msg = msg->next;
    }
    return counter;
}

// Removes the first user with a matching number, if it exists.
void remove_message(struct Message **head, char *number)
{
    if (*head == NULL)
        return;

    if (is_number(number) != 1)
    {
        // printf("'%s' is not a number\n", number);
        return;
    }
    struct Message *msg = *head;
    if (msg->number == atoi(number))
    {
        *head = (*head)->next;
        free(msg->destination);
        free(msg->address);
        free(msg->metadata);
        free(msg->full_text);
        free(msg);
        return;
    }
    while (msg != NULL)
    {
        if (msg->next == NULL)
        {
            return;
        }
        else if ((msg->next->number) == atoi(number))
        {
            struct Message *tmp;
            if (msg->next != NULL)
                tmp = msg->next->next;
            else
                tmp = NULL;
            // printf("Removed message: %d\n", msg->number);
            free(msg->next->destination);
            free(msg->next->address);
            free(msg->next->metadata);
            free(msg->next->full_text);
            free(msg->next);
            msg->next = tmp;
            return;
        }

        msg = msg->next;
    }
    return;
}

/* Creates a new message in heap.*/
struct Message *add_message(struct Message **head, int fd, int number, int counter, char *full_text, char *metadata, char *destination, struct sockaddr_in *address)
{
    struct Message *msg = malloc(sizeof(struct Message));
    if (msg == NULL)
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
        *head = msg;
        msg->next = NULL;
    }
    else
    {
        msg->next = *head;
        *head = msg;
    }
    msg->fd = fd;
    msg->number = number;
    msg->counter = counter;
    char *check = msg->full_text = strdup(full_text);
    check_error_pointer((char *)check, "strdup", fd, init_time, time2, raw_message, NULL, NULL, NULL, received_messages, NULL, user_head, msg_head);
    check = msg->metadata = strdup(metadata);
    check_error_pointer(check, "strdup", fd, init_time, time2, raw_message, NULL, NULL, NULL, received_messages, NULL, user_head, msg_head);
    msg->address = malloc(sizeof(struct sockaddr_in));
    if (msg->address == NULL)
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
    *(msg->address) = *address;
    msg->status = 1;
    check = msg->destination = strdup(destination);
    check_error_pointer(check, "strdup", fd, init_time, time2, raw_message, NULL, NULL, NULL, received_messages, NULL, user_head, msg_head);
    // printf(" - new msg number %i queued for %i to %u\n", number, counter, msg->address->sin_addr.s_addr);
    return msg;
}

// Removes all messages with matching number
void checkout_message(struct Message *head, char *number)
{
    struct Message *msg = head;
    while (msg != NULL)
    {
        if (msg->number == atoi(number))
        {
            msg->status = 0;
            // printf("  - checked out %i\n", msg->number);
        }
        msg = msg->next;
    }
}

// Sends messages from buffer at right time
int check_messages(struct Message **head, struct User **cache_head, int global_counter, int timeout)
{
    struct Message *msg = *head;
    while (msg != NULL)
    {
        if (msg->counter <= global_counter && msg->status != 0)
        {
            if (msg->status == -1) // Deletes user in msg->metadata from cache
            {
                msg->status = 0;
                struct Message *tmp = msg;
                msg = msg->next;
                // printf("User %s removed from cache\n", tmp->metadata);
                if (lookup_user(*cache_head, tmp->metadata) != NULL)
                    remove_user(cache_head, tmp->metadata);
                char *n_str = malloc(1600);
                check_error_pointer(n_str,
                                    "malloc",
                                    (*head)->fd,
                                    init_time,
                                    time2,
                                    raw_message,
                                    NULL,
                                    NULL,
                                    NULL,
                                    received_messages,
                                    NULL,
                                    user_head,
                                    msg_head);
                int check = sprintf(n_str, "%d", tmp->counter);
                check_error(check, "sprintf", (*head)->fd, init_time, time2, raw_message, NULL, NULL, NULL, received_messages, NULL, user_head, msg_head);
                checkout_message(*head, n_str);
                free(n_str);
                continue;
            }
            if (msg->status == -2)
            {
                close(fd);
                free(init_time);
                free(time2);
                free_users(user_head);
                free(raw_message);
                free(receive_addr);
                free(received_messages);
                free_messages(msg_head);
                fprintf(stderr, "Could not contact server\n");
                exit(EXIT_FAILURE);
            }
            // printf("sending packet: %s (%i, %i)\n\n", msg->full_text, msg->counter, global_counter);
            //   send response
            int check = send_packet(msg->fd,
                                    msg->full_text,
                                    (int)strlen(msg->full_text),
                                    0,
                                    (struct sockaddr *)msg->address,
                                    sizeof(struct sockaddr_in));
            if (check == -1)
            {
                close(fd);
                free(init_time);
                free(time2);
                free_users(user_head);
                free(raw_message);
                free(receive_addr);
                free(received_messages);
                free_messages(msg_head);
                printf("Could not send package\n");
                exit(EXIT_FAILURE);
            }
            msg->status = 0;
            msg = msg->next;
        }
        else if ((msg->counter + (8 * timeout)) <= global_counter) // Clears old messages from memory
        {
            struct Message *tmp = msg;
            // printf("Removed old message: %s\n", tmp->full_text);
            msg = msg->next;
            char *n_str = malloc(1600);
            check_error_pointer(n_str, "malloc", fd, init_time, time2, raw_message, NULL, NULL, NULL, received_messages, NULL, user_head, msg_head);
            int check = sprintf(n_str, "%d", tmp->number);
            check_error(check, "sprintf", fd, init_time, time2, raw_message, NULL, NULL, NULL, received_messages, NULL, user_head, msg_head);
            remove_message(head, n_str);
            free(n_str);
        }
        else
            msg = msg->next;
    }
    // print_message_count(*head);
    return 0;
}

// Updates Message's status value
int set_status(struct Message *msg, int status)
{
    if (msg == NULL)
        return -1;
    msg->status = status;
    return 1;
}

// Recursively frees all users. Should be used from head
void free_messages(struct Message *msg)
{
    if (msg == NULL)
    {
        return;
    }
    if (msg->next != NULL)
    {
        free_messages(msg->next);
    }
    free(msg->destination);
    free(msg->address);
    free(msg->metadata);
    free(msg->full_text);
    free(msg);
}