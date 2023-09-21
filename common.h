#ifndef COMMON_H
#define COMMON_H

#define MAX_TEXT 1400
#define BUFSIZE 1600

char *res;
char *msg;
int fd;
struct timespec *init_time;
struct timespec *time2;
char *raw_message;
int *received_messages;
struct User *user_head;
struct Message *msg_head;
struct sockaddr_in *receive_addr;

void free_split_message(char **msg, int *size);

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
                 struct Message *message_head);

void check_error_pointer(char *res,
                         char *msg,
                         int fd,
                         struct timespec *ts,
                         struct timespec *ts2,
                         char *raw_message,
                         char *string2,
                         char *other_string,
                         char **other_string_list,
                         int *received_messages,
                         int *other_int,
                         struct User *user_head,
                         struct Message *message_head);

#endif // COMMON_H