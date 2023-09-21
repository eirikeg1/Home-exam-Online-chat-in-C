#include "header.h"
#include "user_list.h"
#include "common.h"

#define MAX_TEXT 1400
#define BUFSIZE 1600

// returns 1 if valid username and 0 if not. Sets appropriate null byte if username is too long
int check_username(char *username)
{
    unsigned int tmp = (unsigned int)strlen(username);
    // Checks if any of characters are not ASCII
    for (unsigned int i = 0; i < tmp; i++)
    {
        if (strlen(username) > 20)
        {
            return 0;
        }
        if ((int)username[i] < 0)
        {
            return 0;
        }
    }
    return 1;
}

// gets string from keyboard, removes newline at the end and all trailing chars after BUFSIZE
void get_string(char buf[], int size)
{
    char c;
    fgets(buf, size, stdin);

    // removes newline from end of string
    if (buf[strlen(buf) - 1] == '\n')
    {
        buf[strlen(buf) - 1] = '\0';
    }
    // removes trailing characters after BUFSIZE
    else
        while ((c = getchar()) != '\n' && c != EOF)
            ;
}

// Checks if string is number. Returns 1 if yes and 0 if no
int is_number(char *s)
{
    for (int i = 0; s[i] != '\0'; i++)
    {
        if (isdigit(s[i]) == 0)
            return 0;
    }
    return 1;
}

// Splits string into double-pointer array on white-space
int *split(char ***destination, char *message)
{
    int *destination_index = malloc(sizeof(int));
    check_error_pointer((char *)destination_index, "malloc", fd, init_time, time2, raw_message, NULL, NULL, NULL, received_messages, NULL, user_head, msg_head);
    *destination_index = -1;
    int word_index = 0;
    char *tmp = malloc(MAX_TEXT + 1);
    check_error_pointer(tmp, "malloc", fd, init_time, time2, raw_message, NULL, NULL, NULL, received_messages, destination_index, user_head, msg_head);
    for (long unsigned int i = 0; i <= (long unsigned int)strlen(message); i++)
    {
        // printf(" * char: %c, i: %lu, len:%lu\n", message[i], i, strlen(message));
        // Adds nullbyte and starts new word on space or nullbyte values and copies the old word to **destination
        if (isspace(message[i]) || message[i] == 0)
        {
            // Limits the message length to MAX_TEXT chars
            if (word_index > MAX_TEXT)
                word_index = MAX_TEXT;
            tmp[word_index] = 0;
            if (destination != NULL) // If destination is null. Only the counted words are returned
            {
                *destination = realloc(*destination, (++(*destination_index) + 1) * sizeof(char **));
                if (*destination == NULL)
                {
                    printf("realloc destination error");
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
                (*destination)[*destination_index] = strdup(tmp);
                word_index = 0;
            }
        }
        // builds a new string
        else
        {
            tmp[word_index++] = message[i];
        }
    }
    free(tmp);
    (*destination_index)++;
    return destination_index;
}

// Copies everything after keyword 'MSG' in 'char *raw_message' to destination and returns index. Saves both to the heap
void get_msg(char **destination, char *raw_message)
{
    while (*raw_message != '\0')
    {
        if (*raw_message == 'M' &&
            *(raw_message + 1) == 'S' &&
            *(raw_message + 2) == 'G')
        {
            *destination = strdup(raw_message + 4);
            // printf("--------\nMESSAGE:%s\n--------", *destination);
            *destination = realloc(*destination, strlen(*destination) + 1);
            if (*destination == NULL)
            {
                fprintf(stderr, "Realloc failed in function 'get_msg()'");
            }
            return;
        }
        raw_message++;
    }
    return;
}
