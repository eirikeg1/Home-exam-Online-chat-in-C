
#include "header.h"

#include "memory_management.c"
#include "send_packet.c"
#include "user_list.c"
#include "string_operations.c"
#include "message_list.c"

#define BUFSIZE 1600
struct sockaddr_in *receive_addr;

void check_error(int res, char *msg)
{
    if (res == -1)
    {
        perror(msg);
        /* rydde */
        exit(EXIT_FAILURE);
    }
}
void check_error_char(char *res, char *msg)
{
    if (res == NULL)
    {
        perror(msg);
        /* rydde */
        exit(EXIT_FAILURE);
    }
}

void check_error_int(int *res, char *msg)
{
    if (res == NULL)
    {
        perror(msg);
        /* rydde */
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 6)
    {
        printf("Usage: ./upush_client <nick> <ip-address> <port> <timeout> <loss_probability>\n");
        return EXIT_FAILURE;
    }
    if (strspn(argv[3], "0123456789") != strlen(argv[3]))
    {
        printf("%s is not a number\n", argv[3]);
        printf("Usage: ./upush_client <nick> <ip-address> <port> <timeout> <loss_probability>\n");
        return EXIT_FAILURE;
    }
    if (strspn(argv[4], "0123456789") != strlen(argv[4]))
    {
        printf("%s is not a number\n", argv[4]);
        printf("Usage: ./upush_client <nick> <ip-address> <port> <timeout> <loss_probability>\n");
        return EXIT_FAILURE;
    }
    if (strspn(argv[5], "0123456789") != strlen(argv[5]))
    {
        printf("%s is not a number\n", argv[5]);
        printf("Usage: ./upush_client <nick> <ip-address> <port> <timeout> <loss_probability>\n");
        return EXIT_FAILURE;
    }
    struct timeval t;
    int number = 0;
    int timeout = atoi(argv[4]);
    int heartbeat_amount = 9; // makes sure heartbeat is not sendt more than once every 10 seconds
    int client_fd, check;
    receive_addr = malloc(sizeof(struct sockaddr_in) + BUFSIZE);
    if (receive_addr == NULL)
    {
        perror("malloc");
        /* rydde */
        exit(EXIT_FAILURE);
    }
    socklen_t addr_len = BUFSIZE;
    struct in_addr ip_addr;
    fd_set set;
    struct timespec *init_time = malloc(sizeof(struct timespec));
    if (init_time == NULL)
    {
        perror("malloc");
        /* rydde */
        exit(EXIT_FAILURE);
    }
    check = clock_gettime(CLOCK_REALTIME, init_time);
    check_error(check, "clock_gettime");
    struct timespec *time2 = malloc(sizeof(struct timespec));
    char **msg_list;
    char *raw_msg = malloc(BUFSIZE);
    check_error_char(raw_msg, "malloc");
    // creates clients network info
    struct in_addr inaddr;
    struct sockaddr_in to_client_addr;
    raw_msg[0] = 0;

    // Initialize cached userlist
    struct User *cache_head = NULL;
    struct User *block_head = NULL;
    struct Message *msg_head = NULL;

    int port = atoi(argv[3]);
    float loss_probability = atof(argv[5]) / 100.0;
    // printf("lp:%f\nto:%i\n", loss_probability, timeout);
    set_loss_probability(loss_probability);

    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    check_error(client_fd, "socket");

    check = inet_pton(AF_INET, argv[2], &ip_addr.s_addr);
    check_error(check, "inet_pton");
    if (!check)
    {
        fprintf(stderr, "Invalid IP address: %s", argv[2]);
        return EXIT_FAILURE;
    }
    receive_addr->sin_family = AF_INET;
    receive_addr->sin_port = htons(port); // htons konverterer til network byte order
    receive_addr->sin_addr = ip_addr;

    // Initial server request
    int response_length = snprintf(NULL, 0, "PKT %i REG %s", number, argv[1]);
    check_error(response_length, "snprintf");
    char registration_msg[response_length + 1];
    check = sprintf(registration_msg, "PKT %i REG %s", number++, argv[1]);
    check_error(check, "sprintf");
    registration_msg[response_length] = 0;
    check = send_packet(client_fd,
                        registration_msg,
                        strlen(registration_msg),
                        0,
                        (struct sockaddr *)receive_addr,
                        sizeof(struct sockaddr_in));
    check_error(check, "send_packet");

    add_message(&msg_head,
                client_fd,
                number - 1,
                counter + timeout,
                registration_msg,
                "REG",
                argv[1],
                receive_addr);

    add_message(&msg_head,
                client_fd,
                number - 1,
                counter + (2 * timeout),
                registration_msg,
                "REG",
                argv[1],
                receive_addr);

    struct Message *kill_package = add_message(&msg_head, // If first three registration messages are not awknowledged, the program is terminated
                                               client_fd,
                                               number - 1,
                                               counter + (3 * timeout),
                                               "KILL",
                                               "KILL",
                                               "KILL",
                                               receive_addr);
    set_status(kill_package, -2); // Messages with status 2 kills the program and prints "Could not contact server" to stderr (in message_list.c check_messages())

    int registration = 0;

    // Initial connection
    /* while (registration == 0)
    {
        FD_ZERO(&set);
        fflush(NULL);
        FD_SET(client_fd, &set);

        t.tv_sec = 0;
        t.tv_usec = 500000;

        check = clock_gettime(CLOCK_REALTIME, time2);
        check_error(check, "clock_gettime");
        // prevents stack overflow
        if (counter == INT_MAX)
        {
            int check = clock_gettime(CLOCK_REALTIME, init_time);
            check_error(check, "clock_gettime");
        }
        counter = time2->tv_sec - init_time->tv_sec;

        // Stops program after timeout seconds if no acknowledgement is received from server
        if (counter == timeout)
        {
            fprintf(stderr, "Could not contact server\n");
            exit(EXIT_FAILURE);
        }
        check = select(FD_SETSIZE, &set, NULL, NULL, &t);

        if (FD_ISSET(client_fd, &set))
        {

            check = recvfrom(client_fd,
                             raw_msg,
                             BUFSIZE,
                             0,
                             (struct sockaddr *)receive_addr,
                             &addr_len);
            check_error(check, "read");
            raw_msg[check] = '\0';
            // printf("* received: %s\n\n", raw_msg);

            msg_list = malloc(BUFSIZE);
            if (msg_list == NULL)
            {
                perror("malloc");
                rydde
                exit(EXIT_FAILURE);
            }

            int *size = split(&msg_list, raw_msg);

            if (strcmp(msg_list[0], "ACK") == 0 && // Nick registered
                is_number(msg_list[1]) == 1 &&
                strcmp(msg_list[2], "OK") == 0)
            {

                if (atoi(msg_list[1]) == number - 1)
                {
                    registration = 1;
                }
                checkout_message(msg_head, msg_list[1]);
            }
            free_split_message(msg_list, size);
        }
    }

    Resets counter
    check = clock_gettime(CLOCK_REALTIME, init_time);
    check_error(check, "clock_gettime");
    check = clock_gettime(CLOCK_REALTIME, time2);
    check_error(check, "clock_gettime");
    counter = time2->tv_sec - init_time->tv_sec;
    */

    while (strcmp(raw_msg, "QUIT"))
    {
        FD_ZERO(&set);
        fflush(NULL);
        FD_SET(STDIN_FILENO, &set);
        FD_SET(client_fd, &set);
        t.tv_sec = 0;
        t.tv_usec = 500000;

        receive_addr->sin_family = AF_INET;
        receive_addr->sin_port = htons(port);
        receive_addr->sin_addr = ip_addr;

        check = clock_gettime(CLOCK_REALTIME, time2);
        check_error(check, "clock_gettime");
        // prevents stack overflow
        if (counter == INT_MAX)
        {
            int check = clock_gettime(CLOCK_REALTIME, init_time);
            check_error(check, "clock_gettime");
        }
        counter = time2->tv_sec - init_time->tv_sec;
        check = select(FD_SETSIZE, &set, NULL, NULL, &t);
        check_error(check, "select");

        // Heartbeat: sends new registration message every 10 seconds
        if (counter % 10 == 0 &&
            registration == 1) // Only sends heartbeat if first registration message is received
        {
            if (heartbeat_amount != counter)
            {
                int response_length = snprintf(NULL, 0, "PKT %i REG %s", number, argv[1]);
                check_error(response_length, "snprintf");
                char heartbeat_msg[response_length + 1];
                check = sprintf(heartbeat_msg, "PKT %i REG %s", number++, argv[1]);
                check_error(check, "sprintf");
                heartbeat_msg[response_length] = 0;
                // printf(" * HEARTBEAT %i\n", counter);
                check = send_packet(client_fd,
                                    heartbeat_msg,
                                    strlen(heartbeat_msg),
                                    0,
                                    (struct sockaddr *)receive_addr,
                                    sizeof(struct sockaddr_in));
                check_error(check, "send_packet");
                heartbeat_amount = counter;
                add_message(&msg_head, client_fd, number - 1, counter + (2 * timeout), heartbeat_msg, "REG", argv[1], receive_addr);
                add_message(&msg_head, client_fd, number - 1, counter + timeout, heartbeat_msg, "REG", argv[1], receive_addr);
            }
        }

        // keyboard input
        if (FD_ISSET(STDIN_FILENO, &set))
        {
            get_string(raw_msg, BUFSIZE);
            msg_list = malloc(BUFSIZE);
            if (msg_list == NULL)
            {
                perror("malloc");
                /* rydde */
                exit(EXIT_FAILURE);
            }

            int *msg_size = split(&msg_list, raw_msg);
            if (*raw_msg == '@' && *msg_size >= 2)
            {
                msg_size = split(&msg_list, raw_msg + 1);

                int header_size = 2 + strlen(msg_list[0]);
                char *message = (raw_msg + header_size);

                struct User *blocked = lookup_user(block_head, msg_list[0]);
                struct User *user = lookup_user(cache_head, msg_list[0]);

                if (user != NULL && blocked == NULL) // Found user in cache
                {

                    // printf("FOUND %s IN CACHE\n", user->nick);
                    int response_length = snprintf(NULL,
                                                   0,
                                                   "PKT %i FROM %s TO %s MSG %s",
                                                   number,
                                                   argv[1],
                                                   user->nick,
                                                   message);
                    check_error(response_length, "snprintf");
                    char *new_msg = malloc(response_length + 1);
                    check_error_char(new_msg, "malloc");
                    int check = sprintf(new_msg,
                                        "PKT %i FROM %s TO %s MSG %s",
                                        number++,
                                        argv[1],
                                        user->nick,
                                        message);
                    check_error(check, "sprintf");
                    new_msg[response_length] = 0;
                    // printf(" * * * * New message: %s\n", message);
                    // send response
                    check = send_packet(client_fd,
                                        new_msg,
                                        (int)strlen(new_msg),
                                        0,
                                        (struct sockaddr *)&user->ip,
                                        sizeof(struct sockaddr_in));
                    check_error(check, "send_packet");

                    add_message(&msg_head,
                                client_fd,
                                number - 1,
                                counter + timeout,
                                new_msg,
                                message,
                                user->nick,
                                &user->ip);
                    free(new_msg);
                    response_length = snprintf(NULL, 0, "PKT %i LOOKUP %s", number - 1, msg_list[0]);
                    check_error(response_length, "snprintf");
                    new_msg = malloc(response_length + 1);
                    check_error_char(new_msg, "malloc");
                    check = sprintf(new_msg, "PKT %i LOOKUP %s", number - 1, msg_list[0]);
                    check_error(check, "sprintf");
                    new_msg[response_length] = 0;

                    add_message(&msg_head,
                                client_fd,
                                number - 1,
                                counter + (2 * timeout),
                                new_msg,
                                msg_list[0],
                                user->nick,
                                receive_addr); // adds second lookup call to queue

                    struct Message *tmp = add_message(&msg_head,
                                                      0,
                                                      number - 1,
                                                      counter + (3 * timeout) + 1,
                                                      "KILL",
                                                      user->nick,
                                                      msg_list[0],
                                                      &user->ip); // if second lookup fails, user is removed from cache.
                    set_status(tmp, -1);
                    free(new_msg);
                }
                else // Make lookup call
                {
                    struct User *blocked = lookup_user(block_head, msg_list[0]);
                    if (blocked == NULL)
                    { // printf("Not in cache\n");
                        //  create response
                        int response_length = snprintf(NULL, 0, "PKT %i LOOKUP %s", number, msg_list[0]);
                        check_error(response_length, "snprintf");
                        char *new_msg = malloc(response_length + 1);
                        check_error_char(new_msg, "malloc");
                        int check = sprintf(new_msg, "PKT %i LOOKUP %s", number++, msg_list[0]);
                        check_error(check, "sprintf");
                        new_msg[response_length] = 0;
                        // send response

                        check = send_packet(client_fd,
                                            new_msg,
                                            (int)strlen(new_msg),
                                            0,
                                            (struct sockaddr *)receive_addr,
                                            sizeof(struct sockaddr_in));
                        check_error(check, "send_packet");
                        add_message(&msg_head, client_fd, number - 1, counter + (2 * timeout), new_msg, message, msg_list[0], receive_addr);
                        add_message(&msg_head, client_fd, number - 1, counter + timeout, new_msg, message, msg_list[0], receive_addr);
                        number++;
                        free(new_msg);
                    }

                    // receive acknowledgement
                }
            }
            else if (strcmp(msg_list[0], "BLOCK") == 0 && *msg_size >= 2)
            {
                int check = check_username(msg_list[1]);
                if (check == 1)
                {
                    printf("BLOCKED %s\n", msg_list[1]);
                    add_user(&block_head, msg_list[1], *receive_addr);
                }
                else
                {
                    fprintf(stderr, "%s IS NOT A VALID USERNAME\n", msg_list[1]);
                }
            }
            else if (strcmp(msg_list[0], "UNBLOCK") == 0 && *msg_size >= 2)
            {
                int check = check_username(msg_list[1]);
                if (check == 1)
                {
                    printf("UNBLOCKED %s\n", msg_list[1]);
                    remove_user(&block_head, msg_list[1]);
                }
                else
                {
                    fprintf(stderr, "%s IS NOT A VALID USERNAME\n", msg_list[1]);
                }
            }
            free_split_message(msg_list, msg_size);
        }
        // network input
        else if (FD_ISSET(client_fd, &set))
        {
            check = recvfrom(client_fd,
                             raw_msg,
                             BUFSIZE,
                             0,
                             (struct sockaddr *)receive_addr,
                             &addr_len);
            check_error(check, "read");
            raw_msg[check] = '\0';
            // printf("* received: %s\n\n", raw_msg);

            msg_list = malloc(BUFSIZE);
            if (msg_list == NULL)
            {
                perror("malloc");
                /* rydde */
                exit(EXIT_FAILURE);
            }

            int *size = split(&msg_list, raw_msg);

            char **response = malloc(BUFSIZE);
            if (response == NULL)
            {
                perror("malloc");
                /* rydde */
                exit(EXIT_FAILURE);
            }
            int *response_size = split(&response, raw_msg);

            if (strcmp(response[0], "PKT") == 0 && *response_size > 7) // Message from another client
            {

                // Correct format
                if (
                    is_number(response[1]) == 1 ||
                    strcmp(response[2], "FROM") == 0 ||
                    strcmp(response[4], "TO") == 0 ||
                    strcmp(response[6], "MSG") == 0)

                {
                    if (strcmp(response[5], argv[1]) != 0)
                        fprintf(stderr, "WRONG %s NAME\n", response[1]);

                    struct User *block_check = lookup_user(block_head, response[3]);
                    if (block_check == NULL)
                    {
                        char *msg;
                        get_msg(&msg, raw_msg);
                        printf("%s: %s\n\n", response[3], msg);
                        free(msg);

                        int response_length = snprintf(NULL, 0, "ACK %s OK", response[1]);
                        check_error(response_length, "snprintf");
                        char *new_msg = malloc(response_length + 1);
                        check_error_char(new_msg, "malloc");
                        int check = sprintf(new_msg, "ACK %s OK", response[1]);
                        check_error(check, "sprintf");
                        new_msg[response_length] = 0;

                        //  send response
                        check = send_packet(client_fd,
                                            new_msg,
                                            (int)strlen(new_msg),
                                            0,
                                            (struct sockaddr *)receive_addr,
                                            sizeof(struct sockaddr_in));
                        check_error(check, "send_packet");
                        free(new_msg);
                    }

                    checkout_message(msg_head, response[1]);
                }
                else
                {
                    ;
                }
            }
            else if (strcmp(response[0], "ACK") == 0 && *response_size > 7) // Acknowledgement from server
            {
                if (strcmp(response[2], "NICK") != 0 ||
                    is_number(response[1]) == 0 ||
                    strcmp(response[4], "IP") != 0 ||
                    strcmp(response[6], "PORT") != 0)

                {
                    fprintf(stderr, "WRONG %s FORMAT\n", response[1]);
                }

                check = inet_pton(AF_INET, response[5], &inaddr.s_addr);
                check_error(check, "inet_pton");
                if (!check)
                {
                    fprintf(stderr, "Invalid IP address: %s", response[5]);
                    return EXIT_FAILURE;
                }

                struct User *block_check = lookup_user(block_head, response[3]);
                if (block_check == NULL)
                {
                    to_client_addr.sin_family = AF_INET;
                    to_client_addr.sin_port = htons(atoi(response[7]));
                    to_client_addr.sin_addr = inaddr;

                    // adds user to cached users
                    struct User *user = add_user(&cache_head, response[3], to_client_addr);
                    register_as_cache(user);

                    struct Message *msg_to_client = lookup_message(msg_head, atoi(response[1]));
                    if (msg_to_client != NULL)
                    {
                        // create response
                        int response_length = snprintf(NULL, 0, "PKT %i FROM %s TO %s MSG %s", number, argv[1], user->nick, msg_to_client->metadata);
                        check_error(response_length, "snprintf");
                        char *new_msg = malloc(response_length + 1);
                        check_error_char(new_msg, "malloc");
                        int check = sprintf(new_msg,
                                            "PKT %i FROM %s TO %s MSG %s",
                                            number++,
                                            argv[1],
                                            user->nick,
                                            msg_to_client->metadata);
                        check_error(check, "sprintf");
                        new_msg[response_length] = 0;
                        // printf(" * * * * New message: %s\n", msg_to_client->metadata);
                        //  send response
                        check = send_packet(client_fd,
                                            new_msg,
                                            (int)strlen(new_msg),
                                            0,
                                            (struct sockaddr *)&user->ip,
                                            sizeof(struct sockaddr_in));
                        check_error(check, "send_packet");

                        checkout_message(msg_head, response[1]);

                        add_message(&msg_head,
                                    client_fd,
                                    number - 1,
                                    counter + timeout,
                                    new_msg,
                                    msg_to_client->metadata,
                                    user->nick,
                                    &user->ip);
                        free(new_msg);

                        response_length = snprintf(NULL, 0, "PKT %i LOOKUP %s", number - 1, msg_list[3]);
                        check_error(response_length, "snprintf");
                        new_msg = malloc(response_length + 1);
                        check_error_char(new_msg, "malloc");
                        check = sprintf(new_msg, "PKT %i LOOKUP %s", number - 1, msg_list[3]);
                        check_error(check, "sprintf");
                        new_msg[response_length] = 0;

                        add_message(&msg_head,
                                    client_fd,
                                    number - 1,
                                    counter + (2 * timeout),
                                    new_msg,
                                    msg_to_client->metadata,
                                    msg_list[3],
                                    receive_addr); // adds second lookup call to queue
                        number++;
                        free(new_msg);
                    }
                }
            }
            else if (strcmp(response[0], "ACK") == 0 && // Nick registered
                     is_number(response[1]) == 1 &&
                     strcmp(response[2], "OK") == 0)
            {
                checkout_message(msg_head, response[1]);
                registration == 1;
            }
            else if (strcmp(response[0], "ACK") == 0 && // Nick not registered
                     is_number(response[1]) == 1 &&
                     strcmp(response[2], "NOT") == 0 &&
                     strcmp(response[3], "FOUND") == 0)
            {
                struct Message *msg = lookup_message(msg_head, atoi(response[1]));
                fprintf(stderr, "NICK %s NOT REGISTERED\n", msg->destination);
                checkout_message(msg_head, response[1]);
            }
            else
            {
                checkout_message(msg_head, response[1]);
            }

            free_split_message(response, response_size);
            free_split_message(msg_list, size);
        }
        // printf("Checking messages: %i\n", counter);
        check_messages(&msg_head, &cache_head, counter, timeout);
        // print_message_count(msg_head);
    }

    close(client_fd);
    free(init_time);
    free(time2);
    free(raw_msg);
    free(receive_addr);
    free_users(cache_head);
    free_messages(msg_head);
    return EXIT_SUCCESS;
}
