#include "header.h"
#include "common.h"
#include "user_list.h"
#include "message_list.h"

#define MAX_TEXT 1400
#define BUFSIZE 1600
// Inits which will be used in error_check methods. These need to be global so they are malloced in all files
int fd;
struct sockaddr_in *receive_addr;
struct User *user_head = NULL;
struct User *block_head = NULL;
struct Message *msg_head = NULL;
int *received_messages;
struct timespec *init_time;
struct timespec *time2;
char *raw_message;

int counter;

#define _GNU_SOURCE

void free_split_message(char **msg, int *size)
{
    char **tmp = msg;
    for (int i = 0; i < *size; i++)
    {
        free(tmp[i]);
    }
    free(size);
    free(msg);
}

/* Free's all variables if error happened. Har arguments for the most important variables, plus a few extra in case it happens after
another malloc. */
void check_error(int res,
                 char *msg,
                 int fd,
                 struct timespec *ts,
                 struct timespec *ts2,
                 char *raw_message,
                 char *string2,
                 char *string3,
                 char **string_list,
                 int *received_messages,
                 int *int2,
                 struct User *user_head,
                 struct Message *message_head)
{
    if (res == -1)
    {
        perror(msg);
        /* rydde */
        if (fd != -1)
            close(fd);
        if (ts != NULL)
            free(ts);
        if (ts2 != NULL)
            free(ts2);
        if (raw_message != NULL)
            free(raw_message);
        if (string2 != NULL)
            free(string2);
        if (string3 != NULL)
            free(string3);
        if (string_list != NULL)
            free(string_list);
        if (received_messages != NULL)
            free(received_messages);
        if (int2 != NULL)
            free(int2);
        if (user_head != NULL)
            free_users(user_head);
        if (message_head != NULL)
            free_messages(message_head);
        exit(EXIT_FAILURE);
    }
}

/* Free's all variables if error happened. Har arguments for the most important variables, plus a few extra in case it happens after
another malloc. */
void check_error_pointer(char *res,
                         char *msg,
                         int fd,
                         struct timespec *ts,
                         struct timespec *ts2,
                         char *raw_message,
                         char *string2,
                         char *string3,
                         char **string_list,
                         int *received_messages,
                         int *int2,
                         struct User *user_head,
                         struct Message *message_head)
{
    if (res == NULL)
    {
        perror(msg);
        /* rydde */
        if (fd != -1)
            close(fd);
        if (ts != NULL)
            free(ts);
        if (ts2 != NULL)
            free(ts2);
        if (raw_message != NULL)
            free(raw_message);
        if (string2 != NULL)
            free(string2);
        if (string3 != NULL)
            free(string3);
        if (string_list != NULL)
            free(string_list);
        if (received_messages != NULL)
            free(received_messages);
        if (int2 != NULL)
            free(int2);
        if (user_head != NULL)
            free_users(user_head);
        if (message_head != NULL)
            free_messages(message_head);
        exit(EXIT_FAILURE);
    }
}

void free_message(char **msg, int *size)
{
    char **tmp = msg;
    for (int i = 0; i < *size; i++)
    {
        free(tmp[i]);
    }
    free(size);
    free(msg);
}