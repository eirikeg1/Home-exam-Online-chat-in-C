
#include "header.h"

#include "common.c"
#include "send_packet.c"
#include "user_list.c"
#include "string_operations.c"
#include "message_list.c"

#define IP "172.18.146.247"
#define BUFSIZE 1600

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage: ./upush_server <port> <loss_probability> %i\n", argc);
        return EXIT_FAILURE;
    }
    if (strspn(argv[1], "0123456789") != strlen(argv[1]))
    {
        printf("%s is not a number\n", argv[1]);
        printf("Usage: ./upush_server <port> <loss_probability> %i\n", argc);
        return EXIT_FAILURE;
    }
    if (strspn(argv[2], "0123456789") != strlen(argv[2]))
    {
        printf("%s is not a number\n", argv[2]);
        printf("Usage: ./upush_server <port> <loss_probability>\n");
        return EXIT_FAILURE;
    }

    // Not used, but are free'ed in utility-files also used by upush_client.c
    receive_addr = malloc(0);
    received_messages = malloc(0);

    char *raw_message = malloc(BUFSIZE);
    check_error_pointer(raw_message, "malloc", -1, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, user_head, msg_head);
    *raw_message = 0;
    int rc;
    char **message;
    struct sockaddr_in server_addr, client_addr;
    struct timespec *init_time = malloc(sizeof(struct timespec));
    check_error_pointer(raw_message, "malloc", -1, NULL, NULL, raw_message, NULL, NULL, NULL, NULL, NULL, user_head, msg_head);
    int check = clock_gettime(CLOCK_REALTIME, init_time);
    check_error(check, "clock_gettime", -1, init_time, NULL, raw_message, NULL, NULL, NULL, NULL, NULL, user_head, msg_head);
    struct timespec *time2 = malloc(sizeof(struct timespec));
    check_error_pointer(raw_message, "malloc", -1, init_time, NULL, raw_message, NULL, NULL, NULL, NULL, NULL, user_head, msg_head);

    fd_set set;
    socklen_t addr_len = BUFSIZE;

    int port = atoi(argv[1]);

    set_loss_probability(atof(argv[2]) / 100.0);

    // Set up socket
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    check_error(fd, "socket", -1, init_time, time2, raw_message, NULL, NULL, NULL, NULL, NULL, user_head, msg_head);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    rc = bind(fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
    check_error(rc, "bind", -1, init_time, time2, raw_message, NULL, NULL, NULL, NULL, NULL, user_head, msg_head);

    rc = inet_pton(AF_INET, IP, &server_addr);
    check_error(rc, "inet_pton", -1, init_time, time2, raw_message, NULL, NULL, NULL, NULL, NULL, user_head, msg_head);
    check_error(rc, "malloc", -1, init_time, time2, raw_message, NULL, NULL, NULL, NULL, NULL, user_head, msg_head);

    printf("Write 'STOP' to stop server\n");

    // Main while loop, runs until user writes 'QUIT'. Runs every second
    while (strcmp(raw_message, "STOP"))
    {
        FD_ZERO(&set);
        fflush(NULL);
        FD_SET(STDIN_FILENO, &set);
        FD_SET(fd, &set);
        struct timeval t;
        t.tv_sec = 1;
        t.tv_usec = 0;

        check = clock_gettime(CLOCK_REALTIME, time2);
        check_error(check, "clock_gettime", -1, init_time, time2, raw_message, NULL, NULL, NULL, NULL, NULL, user_head, msg_head);
        // prevents stack overflow
        if (counter == INT_MAX)
        {
            int check = clock_gettime(CLOCK_REALTIME, init_time);
            check_error(check, "clock_gettime", -1, init_time, time2, raw_message, NULL, NULL, NULL, NULL, NULL, user_head, msg_head);
        }
        counter = time2->tv_sec - init_time->tv_sec;

        rc = select(FD_SETSIZE, &set, NULL, NULL, &t);
        check_error(rc, "select", -1, init_time, time2, raw_message, NULL, NULL, NULL, NULL, NULL, user_head, msg_head);

        // keyboard input
        if (FD_ISSET(STDIN_FILENO, &set))
        {
            get_string(raw_message, BUFSIZE);
        }
        // network input
        else if (FD_ISSET(fd, &set))
        {
            // rc = read(fd, raw_message, BUFSIZE);
            rc = recvfrom(fd,
                          raw_message,
                          BUFSIZE,
                          0,
                          (struct sockaddr *)&client_addr,
                          &addr_len);
            check_error(rc, "read", -1, init_time, time2, raw_message, NULL, NULL, NULL, NULL, NULL, user_head, msg_head);
            raw_message[rc] = '\0';

            // printf("Received: '%s'\n", raw_message);
            //  Set up return address

            message = malloc(BUFSIZE);
            check_error_pointer((char *)message, "malloc", -1, init_time, time2, raw_message, NULL, NULL, NULL, NULL, NULL, user_head, msg_head);
            int *size = split(&message, raw_message);

            if (*size < 3 || is_number(message[1]) == 0)
            {
                ;
            }
            else if (strcmp(message[0], "PKT") == 0) // Packet
            {
                if (strcmp(message[2], "REG") == 0) // Registration message
                {

                    check = check_username(message[3]);
                    if (check == 1)
                    {
                        char *response_msg = malloc(8 + strlen(message[1]));
                        check_error_pointer(response_msg, "malloc", fd, NULL, NULL, raw_message, NULL, NULL, message, NULL, NULL, user_head, msg_head);
                        add_user(&user_head, message[3], client_addr);
                        // create response
                        check = sprintf(response_msg, "ACK %s OK", message[1]);
                        check_error(check, "sprintf", -1, init_time, time2, raw_message, response_msg, NULL, message, NULL, NULL, user_head, msg_head);

                        // send response
                        check = send_packet(fd,
                                            response_msg,
                                            (int)strlen(response_msg),
                                            0,
                                            (struct sockaddr *)&client_addr,
                                            sizeof(struct sockaddr_in));
                        check_error(check, "send_packet", -1, init_time, time2, raw_message, response_msg, NULL, message, NULL, NULL, user_head, msg_head);
                        free(response_msg);
                    }
                    else
                    {
                        // create response
                        char *response_msg = malloc(16 + strlen(message[1]));
                        check_error_pointer(response_msg, "malloc", fd, NULL, NULL, raw_message, NULL, NULL, message, NULL, NULL, user_head, msg_head);

                        int check = sprintf(response_msg, "ACK %s WRONG NAME", message[1]);
                        check_error(check, "sprintf", -1, init_time, time2, raw_message, NULL, NULL, message, NULL, NULL, user_head, msg_head);
                        // send response
                        send_packet(fd,
                                    response_msg,
                                    (int)strlen(response_msg),
                                    0,
                                    (struct sockaddr *)&client_addr,
                                    sizeof(struct sockaddr_in));
                        free(response_msg);
                    }
                }
                else if (strcmp(message[2], "LOOKUP") == 0) // Lookup message
                {
                    int check = check_username(message[3]);
                    if (check == 1)
                    {
                        struct User *user = lookup_user(user_head, message[3]);
                        if (user)
                        {
                            // create response
                            char ip_addr_str[INET_ADDRSTRLEN];
                            const char *inet_check = inet_ntop(AF_INET,
                                                               &user->ip.sin_addr,
                                                               ip_addr_str,
                                                               sizeof(ip_addr_str));
                            check_error((inet_check == NULL), "inet_ntop", -1, init_time, time2, raw_message, NULL, NULL, message, NULL, NULL, user_head, msg_head);
                            unsigned int port = (unsigned int)htons(user->ip.sin_port);

                            int response_length = snprintf(NULL,
                                                           0,
                                                           "ACK %s NICK %s IP %s PORT %u",
                                                           message[1],
                                                           message[3],
                                                           ip_addr_str,
                                                           port);
                            check_error(response_length, "snprintf", -1, init_time, time2, raw_message, NULL, NULL, message, NULL, NULL, user_head, msg_head);
                            char *response_msg = malloc(response_length + 1);
                            check_error_pointer(response_msg, "malloc", -1, init_time, time2, raw_message, NULL, NULL, message, NULL, NULL, user_head, msg_head);
                            int check = sprintf(response_msg,
                                                "ACK %s NICK %s IP %s PORT %u",
                                                message[1],
                                                message[3],
                                                ip_addr_str,
                                                port);
                            check_error(check, "sprintf", -1, init_time, time2, raw_message, NULL, NULL, message, NULL, NULL, user_head, msg_head);
                            // send response
                            check = send_packet(fd,
                                                response_msg,
                                                (int)strlen(response_msg),
                                                0,
                                                (struct sockaddr *)&client_addr,
                                                sizeof(struct sockaddr_in));
                            check_error(check, "send_packet", -1, init_time, time2, raw_message, NULL, NULL, message, NULL, NULL, user_head, msg_head);
                            free(response_msg);
                        }
                        else
                        {
                            // create response
                            char *response_msg = malloc(15 + strlen(message[1]));
                            check_error_pointer(response_msg, "malloc", -1, init_time, time2, raw_message, NULL, NULL, message, NULL, NULL, user_head, msg_head);
                            int check = sprintf(response_msg, "ACK %s NOT FOUND", message[1]);
                            check_error(check, "sprintf", -1, init_time, time2, raw_message, NULL, NULL, message, NULL, NULL, user_head, msg_head);
                            // send response
                            check = send_packet(fd,
                                                response_msg,
                                                (int)strlen(response_msg),
                                                0,
                                                (struct sockaddr *)&client_addr,
                                                sizeof(struct sockaddr_in));
                            check_error(check, "send_packet", -1, init_time, time2, raw_message, NULL, NULL, message, NULL, NULL, user_head, msg_head);
                            // printf("SENDT: %s\n", response_msg);
                            free(response_msg);
                        }
                    }
                }
                else
                {
                    // create response
                    char *response_msg = malloc(18 + strlen(message[1]));
                    check_error_pointer(response_msg, "malloc", -1, init_time, time2, raw_message, NULL, NULL, message, NULL, NULL, user_head, msg_head);
                    int check = sprintf(response_msg, "ACK %s WRONG FORMAT", message[1]);
                    check_error(check, "sprintf", -1, init_time, time2, raw_message, response_msg, NULL, message, NULL, NULL, user_head, msg_head);
                    // send response
                    check = send_packet(fd,
                                        response_msg,
                                        (int)strlen(response_msg),
                                        0,
                                        (struct sockaddr *)&client_addr,
                                        sizeof(struct sockaddr_in));
                    check_error(check, "send_packet", -1, init_time, time2, raw_message, NULL, NULL, message, NULL, NULL, user_head, msg_head);
                    free(response_msg);
                }
            }
            else
            {
                // create response
                char *response_msg = malloc(18 + strlen(message[1]));
                check_error_pointer(response_msg, "malloc", -1, init_time, time2, raw_message, NULL, NULL, message, NULL, NULL, user_head, msg_head);
                int check = sprintf(response_msg,
                                    "ACK %s WRONG FORMAT",
                                    message[1]);
                check_error(check, "sprintf", -1, init_time, time2, raw_message, NULL, NULL, message, NULL, NULL, user_head, msg_head);
                response_msg[17 + strlen(message[1])] = 0;
                // send response
                check = send_packet(fd,
                                    response_msg,
                                    (int)strlen(response_msg),
                                    0,
                                    (struct sockaddr *)&client_addr,
                                    sizeof(struct sockaddr_in));
                check_error(check, "send_packet", -1, init_time, time2, raw_message, NULL, NULL, message, NULL, NULL, user_head, msg_head);
                free(response_msg);
            }
            free_split_message(message, size);
        }
    }

    close(fd);
    free(init_time);
    free(time2);
    free(raw_message);
    free(receive_addr);
    free(received_messages);
    free_users(user_head);
    free_users(block_head);
    free_messages(msg_head);

    return EXIT_SUCCESS;
}
