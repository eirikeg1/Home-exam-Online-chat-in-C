
#include "header.h"

#include "common.c"
#include "send_packet.c"
#include "user_list.c"
#include "string_operations.c"
#include "message_list.c"

#define BUFSIZE 1600
struct sockaddr_in *receive_addr;

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
    received_messages = malloc(BUFSIZE);
    int received_messages_count = 0;
    int number = 0;
    int timeout = atoi(argv[4]);
    int heartbeat_amount = 9; // makes sure heartbeat is not sendt more than once every 10 seconds
    int check;
    receive_addr = malloc(sizeof(struct sockaddr_in) + BUFSIZE);
    if (receive_addr == NULL)
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
    socklen_t addr_len = BUFSIZE;
    struct in_addr ip_addr;
    fd_set set;
    init_time = malloc(sizeof(struct timespec));
    // Can be casted because value is only compared to null and all pointers are the same
    check_error_pointer((char *)init_time,
                        "clock_gettime",
                        -1,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        received_messages,
                        NULL,
                        user_head,
                        msg_head);
    time2 = malloc(sizeof(struct timespec));
    check = clock_gettime(CLOCK_REALTIME, init_time);
    check_error(check,
                "clock_gettime",
                -1,
                init_time,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                received_messages,
                NULL,
                user_head,
                msg_head);
    char **msg_list;
    raw_message = malloc(BUFSIZE);
    check_error_pointer(raw_message,
                        "malloc",
                        -1,
                        init_time,
                        time2,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        received_messages,
                        NULL,
                        user_head,
                        msg_head);
    // creates clients network info
    struct in_addr inaddr;
    struct sockaddr_in to_client_addr;
    raw_message[0] = 0;

    int port = atoi(argv[3]);
    float loss_probability = atof(argv[5]) / 100.0;
    // printf("lp:%f\nto:%i\n", loss_probability, timeout);
    set_loss_probability(loss_probability);

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    check_error(fd,
                "socket",
                -1,
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

    check = inet_pton(AF_INET, argv[2], &ip_addr.s_addr);
    check_error(check,
                "inet_pton",
                -1,
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
    if (!check)
    {
        close(fd);
        free(init_time);
        free(time2);
        free_users(user_head);
        free(raw_message);
        free(receive_addr);
        free(received_messages);
        free_messages(msg_head);
        fprintf(stderr, "Invalid IP address: %s", argv[2]);
        return EXIT_FAILURE;
    }
    // Sets receive_addr to server address
    receive_addr->sin_family = AF_INET;
    receive_addr->sin_port = htons(port); // htons konverterer til network byte order
    receive_addr->sin_addr = ip_addr;

    // Initial server request
    int response_length = snprintf(NULL, 0, "PKT %i REG %s", number, argv[1]);
    check_error(response_length,
                "snprintf",
                -1,
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
    char registration_msg[response_length + 1];
    check = sprintf(registration_msg, "PKT %i REG %s", number++, argv[1]);
    check_error(check,
                "sprintf",
                -1,
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
    registration_msg[response_length] = 0;
    struct Message *kill_package = add_message(&msg_head, // If first three registration messages are not awknowledged, the program is terminated
                                               fd,
                                               number - 1,
                                               counter + (3 * timeout),
                                               "KILL",
                                               "KILL",
                                               "KILL",
                                               receive_addr);
    set_status(kill_package, -2); // Messages with status 2 kills the program and prints "Could not contact server" to stderr (check_messages() in message_list.c)
    add_message(&msg_head,
                fd,
                number - 1,
                counter + (2 * timeout),
                registration_msg,
                "REG",
                argv[1],
                receive_addr);
    add_message(&msg_head,
                fd,
                number - 1,
                counter + timeout,
                registration_msg,
                "REG",
                argv[1],
                receive_addr);
    add_message(&msg_head,
                fd,
                number - 1,
                counter,
                registration_msg,
                "REG",
                argv[1],
                receive_addr);

    int registration = 0;

    printf("Usage: '@<nick> <msg>' to send a message, 'QUIT' to quit\n");

    while (strcmp(raw_message, "QUIT"))
    {
        FD_ZERO(&set);
        fflush(NULL);
        FD_SET(STDIN_FILENO, &set);
        FD_SET(fd, &set);
        t.tv_sec = 0;
        t.tv_usec = 500000;

        receive_addr->sin_family = AF_INET;
        receive_addr->sin_port = htons(port);
        receive_addr->sin_addr = ip_addr;

        check = clock_gettime(CLOCK_REALTIME, time2);
        check_error(check,
                    "clock_gettime",
                    -1,
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
        // prevents stack overflow
        if (counter == INT_MAX)
        {
            int check = clock_gettime(CLOCK_REALTIME, init_time);
            check_error(check,
                        "clock_gettime",
                        -1,
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
        }
        counter = time2->tv_sec - init_time->tv_sec;
        check = select(FD_SETSIZE, &set, NULL, NULL, &t);
        check_error(check,
                    "select",
                    -1,
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

        // Heartbeat: sends new registration message every 10 seconds
        if (counter % 10 == 0 &&
            registration == 1) // Only sends heartbeat if first registration message is received
        {
            if (heartbeat_amount != counter)
            {
                int response_length = snprintf(NULL, 0, "PKT %i REG %s", number, argv[1]);
                check_error(response_length,
                            "snprintf",
                            -1,
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
                char heartbeat_msg[response_length + 1];
                check = sprintf(heartbeat_msg, "PKT %i REG %s", number++, argv[1]);
                check_error(check,
                            "sprintf",
                            -1,
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
                heartbeat_msg[response_length] = 0;
                // printf(" * HEARTBEAT %i\n", counter);
                heartbeat_amount = counter;
                add_message(&msg_head, fd, number - 1, counter + (2 * timeout), heartbeat_msg, "REG", argv[1], receive_addr);
                add_message(&msg_head, fd, number - 1, counter + timeout, heartbeat_msg, "REG", argv[1], receive_addr);
                add_message(&msg_head, fd, number - 1, counter, heartbeat_msg, "REG", argv[1], receive_addr);
            }
        }

        // keyboard input
        if (FD_ISSET(STDIN_FILENO, &set))
        {
            get_string(raw_message, BUFSIZE);
            msg_list = malloc(BUFSIZE * sizeof(char *));
            if (msg_list == NULL)
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

            int *msg_size = split(&msg_list, raw_message);
            if (*raw_message == '@' && *msg_size >= 2)
            {
                free_split_message(msg_list, msg_size);
                msg_list = malloc(BUFSIZE * sizeof(char *));
                msg_size = split(&msg_list, raw_message + 1);

                int header_size = 2 + strlen(msg_list[0]);
                char *message = (raw_message + header_size);

                struct User *blocked = lookup_blocked_user(block_head, msg_list[0]);
                struct User *user = lookup_user(user_head, msg_list[0]);

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
                    check_error(response_length,
                                "snprintf",
                                -1,
                                init_time,
                                time2,
                                raw_message,
                                NULL,
                                NULL,
                                msg_list,
                                received_messages,
                                msg_size,
                                user_head,
                                msg_head);
                    char *new_msg = malloc(response_length + 1);
                    check_error_pointer(new_msg,
                                        "malloc",
                                        fd,
                                        init_time,
                                        time2,
                                        raw_message,
                                        NULL,
                                        NULL,
                                        msg_list,
                                        received_messages,
                                        msg_size,
                                        user_head,
                                        msg_head);
                    int check = sprintf(new_msg,
                                        "PKT %i FROM %s TO %s MSG %s",
                                        number++,
                                        argv[1],
                                        user->nick,
                                        message);
                    check_error(check,
                                "sprintf",
                                fd,
                                init_time,
                                time2,
                                raw_message,
                                new_msg,
                                NULL,
                                msg_list,
                                received_messages,
                                msg_size,
                                user_head,
                                msg_head);
                    new_msg[response_length] = 0;
                    // printf(" * * * * New message: %s\n", message);
                    // send msg_list

                    response_length = snprintf(NULL, 0, "PKT %i LOOKUP %s", number - 1, msg_list[0]);
                    check_error(response_length,
                                "snprintf",
                                fd,
                                init_time,
                                time2,
                                raw_message,
                                new_msg,
                                NULL,
                                msg_list,
                                received_messages,
                                msg_size,
                                user_head,
                                msg_head);
                    char *lookup_msg = malloc(response_length + 1);
                    check_error_pointer(lookup_msg,
                                        "malloc",
                                        fd,
                                        init_time,
                                        time2,
                                        raw_message,
                                        new_msg,
                                        NULL,
                                        msg_list,
                                        received_messages,
                                        msg_size,
                                        user_head,
                                        msg_head);
                    check = sprintf(lookup_msg, "PKT %i LOOKUP %s", number - 1, msg_list[0]);
                    check_error(check,
                                "sprintf",
                                fd,
                                init_time,
                                time2,
                                raw_message,
                                new_msg,
                                NULL,
                                msg_list,
                                received_messages,
                                msg_size,
                                user_head,
                                msg_head);
                    lookup_msg[response_length] = 0;

                    struct Message *tmp = add_message(&msg_head,
                                                      0,
                                                      number - 1,
                                                      counter + (3 * timeout) + 1,
                                                      "KILL",
                                                      user->nick,
                                                      msg_list[0],
                                                      &user->ip); // if second lookup fails, user is removed from cache.
                    set_status(tmp, -1);

                    add_message(&msg_head,
                                fd,
                                number - 1,
                                counter + (2 * timeout),
                                lookup_msg,
                                msg_list[0],
                                user->nick,
                                receive_addr); // adds second lookup call to queue

                    add_message(&msg_head,
                                fd,
                                number - 1,
                                counter + timeout,
                                new_msg,
                                message,
                                user->nick,
                                &user->ip);

                    add_message(&msg_head,
                                fd,
                                number - 1,
                                counter,
                                new_msg,
                                message,
                                user->nick,
                                &user->ip);
                    free(new_msg);

                    free(lookup_msg);
                }
                else // Make lookup call
                {
                    struct User *blocked = lookup_blocked_user(block_head, msg_list[0]);
                    if (blocked == NULL)
                    { // printf("Not in cache\n");
                        //  create msg_list
                        int response_length = snprintf(NULL, 0, "PKT %i LOOKUP %s", number, msg_list[0]);
                        check_error(response_length,
                                    "snprintf",
                                    fd,
                                    init_time,
                                    time2,
                                    raw_message,
                                    NULL,
                                    NULL,
                                    msg_list,
                                    received_messages,
                                    msg_size,
                                    user_head,
                                    msg_head);
                        char *new_msg = malloc(response_length + 1);
                        check_error_pointer(new_msg,
                                            "malloc",
                                            fd,
                                            init_time,
                                            time2,
                                            raw_message,
                                            NULL,
                                            new_msg,
                                            NULL,
                                            received_messages,
                                            NULL,
                                            user_head,
                                            msg_head);
                        int check = sprintf(new_msg, "PKT %i LOOKUP %s", number++, msg_list[0]);
                        check_error(check,
                                    "sprintf",
                                    fd,
                                    init_time,
                                    time2,
                                    raw_message,
                                    NULL,
                                    NULL,
                                    msg_list,
                                    received_messages,
                                    msg_size,
                                    user_head,
                                    msg_head);
                        new_msg[response_length] = 0;
                        // send msg_list

                        add_message(&msg_head, fd, number - 1, counter + (2 * timeout), new_msg, message, msg_list[0], receive_addr);
                        add_message(&msg_head, fd, number - 1, counter + timeout, new_msg, message, msg_list[0], receive_addr);
                        add_message(&msg_head, fd, number - 1, counter, new_msg, message, msg_list[0], receive_addr);
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
        else if (FD_ISSET(fd, &set))
        {
            check = recvfrom(fd,
                             raw_message,
                             BUFSIZE,
                             0,
                             (struct sockaddr *)receive_addr,
                             &addr_len);
            check_error(check,
                        "read",
                        fd,
                        init_time,
                        time2,
                        raw_message,
                        NULL,
                        NULL,
                        msg_list,
                        received_messages,
                        NULL,
                        user_head,
                        msg_head);
            raw_message[check] = '\0';
            // printf("* received: %s from %s\n\n", raw_message, inet_ntoa(receive_addr->sin_addr));

            msg_list = malloc(BUFSIZE);
            if (msg_list == NULL)
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

            int *msg_size = split(&msg_list, raw_message);

            if (strcmp(msg_list[0], "PKT") == 0 && *msg_size > 7) // Message from another client
            {

                // Correct format
                if (
                    is_number(msg_list[1]) == 1 ||
                    strcmp(msg_list[2], "FROM") == 0 ||
                    strcmp(msg_list[4], "TO") == 0 ||
                    strcmp(msg_list[6], "MSG") == 0)

                {
                    if (strcmp(msg_list[5], argv[1]) != 0)
                        fprintf(stderr, "WRONG %s NAME\n", msg_list[1]);

                    checkout_message(msg_head, msg_list[1]);

                    int counter = 0;
                    for (int i = 0; i < received_messages_count; i++)
                    {
                        if (received_messages[i] == atoi(msg_list[1]))
                        {
                            counter = 1;
                            break;
                        }
                    }

                    struct User *block_check = lookup_blocked_user(block_head, msg_list[3]);

                    if (counter == 0 && block_check == NULL)
                    {
                        char *msg;
                        get_msg(&msg, raw_message);
                        printf("%45s: %s\n\n", msg_list[3], msg);
                        received_messages_count++;
                        if (received_messages_count == BUFSIZE)
                            received_messages_count = 1;
                        received_messages[received_messages_count - 1] = atoi(msg_list[1]);
                        free(msg);
                    }

                    int response_length = snprintf(NULL, 0, "ACK %s OK", msg_list[1]);
                    check_error(response_length,
                                "snprintf",
                                fd,
                                init_time,
                                time2,
                                raw_message,
                                NULL,
                                NULL,
                                msg_list,
                                received_messages,
                                msg_size,
                                user_head,
                                msg_head);
                    char *new_msg = malloc(response_length + 1);
                    check_error_pointer(new_msg,
                                        "malloc",
                                        fd,
                                        init_time,
                                        time2,
                                        raw_message,
                                        NULL,
                                        NULL,
                                        msg_list,
                                        received_messages,
                                        msg_size,
                                        user_head,
                                        msg_head);
                    int check = sprintf(new_msg, "ACK %s OK", msg_list[1]);
                    check_error(check,
                                "sprintf",
                                fd,
                                init_time,
                                time2,
                                raw_message,
                                new_msg,
                                NULL,
                                msg_list,
                                received_messages,
                                msg_size,
                                user_head,
                                msg_head);
                    new_msg[response_length] = 0;

                    add_message(&msg_head,
                                fd,
                                atoi(msg_list[1]),
                                counter,
                                new_msg,
                                new_msg,
                                new_msg,
                                receive_addr);
                    free(new_msg);
                }
                else
                {
                    ;
                }
            }
            else if (strcmp(msg_list[0], "ACK") == 0 && *msg_size > 7) // Acknowledgement from server
            {
                if (strcmp(msg_list[2], "NICK") != 0 ||
                    is_number(msg_list[1]) == 0 ||
                    strcmp(msg_list[4], "IP") != 0 ||
                    strcmp(msg_list[6], "PORT") != 0)

                {
                    fprintf(stderr, "WRONG %s FORMAT\n", msg_list[1]);
                }

                check = inet_pton(AF_INET, msg_list[5], &inaddr.s_addr);
                check_error(check,
                            "inet_pton",
                            fd,
                            init_time,
                            time2,
                            raw_message,
                            NULL,
                            NULL,
                            msg_list,
                            received_messages,
                            msg_size,
                            user_head,
                            msg_head);
                if (!check)
                {
                    close(fd);
                    free(init_time);
                    free(time2);
                    free_users(user_head);
                    free(raw_message);
                    free(receive_addr);
                    free(received_messages);
                    free_messages(msg_head);
                    fprintf(stderr, "Invalid IP address: %s", msg_list[5]);
                    return EXIT_FAILURE;
                }

                struct User *block_check = lookup_blocked_user(block_head, msg_list[3]);
                if (block_check == NULL)
                {
                    to_client_addr.sin_family = AF_INET;
                    to_client_addr.sin_port = htons(atoi(msg_list[7]));
                    to_client_addr.sin_addr = inaddr;

                    // adds user to cached users
                    struct User *user = add_user(&user_head, msg_list[3], to_client_addr);
                    register_as_cache(user);

                    struct Message *msg_to_client = lookup_message(&msg_head, atoi(msg_list[1]));
                    if (msg_to_client != NULL)
                    {
                        // create msg_list
                        int response_length = snprintf(NULL, 0, "PKT %i FROM %s TO %s MSG %s", atoi(msg_list[1]), argv[1], user->nick, msg_to_client->metadata);
                        check_error(response_length,
                                    "snprintf",
                                    fd,
                                    init_time,
                                    time2,
                                    raw_message,
                                    NULL,
                                    NULL,
                                    msg_list,
                                    received_messages,
                                    msg_size,
                                    user_head,
                                    msg_head);
                        char *new_msg = malloc(response_length + 1);
                        check_error_pointer(new_msg,
                                            "malloc",
                                            fd,
                                            init_time,
                                            time2,
                                            raw_message,
                                            NULL,
                                            NULL,
                                            msg_list,
                                            received_messages,
                                            NULL,
                                            user_head,
                                            msg_head);
                        int check = sprintf(new_msg,
                                            "PKT %i FROM %s TO %s MSG %s",
                                            atoi(msg_list[1]),
                                            argv[1],
                                            user->nick,
                                            msg_to_client->metadata);
                        check_error(check,
                                    "sprintf",
                                    fd,
                                    init_time,
                                    time2,
                                    raw_message,
                                    NULL,
                                    NULL,
                                    msg_list,
                                    received_messages,
                                    msg_size,
                                    user_head,
                                    msg_head);
                        new_msg[response_length] = 0;
                        // printf(" * * * * New message: %s\n", msg_to_client->metadata);

                        checkout_message(msg_head, msg_list[1]);

                        int *amount = count_message_number(&msg_head, atoi(msg_list[1]));
                        if (*amount < 4)

                        {
                            add_message(&msg_head,
                                        fd,
                                        atoi(msg_list[1]),
                                        counter + (2 * timeout),
                                        new_msg,
                                        msg_to_client->metadata,
                                        user->nick,
                                        &user->ip);
                            add_message(&msg_head,
                                        fd,
                                        atoi(msg_list[1]),
                                        counter + timeout,
                                        new_msg,
                                        msg_to_client->metadata,
                                        user->nick,
                                        &user->ip);
                        }

                        free(amount);

                        add_message(&msg_head,
                                    fd,
                                    atoi(msg_list[1]),
                                    counter,
                                    new_msg,
                                    msg_to_client->metadata,
                                    user->nick,
                                    &user->ip);
                        free(new_msg);
                    }
                }
            }
            else if (strcmp(msg_list[0], "ACK") == 0 && // Nick registered
                     is_number(msg_list[1]) == 1 &&
                     strcmp(msg_list[2], "OK") == 0)
            {
                checkout_message(msg_head, msg_list[1]);
                registration = 1;
            }
            else if (strcmp(msg_list[0], "ACK") == 0 && // Nick not registered
                     is_number(msg_list[1]) == 1 &&
                     strcmp(msg_list[2], "NOT") == 0 &&
                     strcmp(msg_list[3], "FOUND") == 0)
            {
                struct Message *msg = lookup_message(&msg_head, atoi(msg_list[1]));
                fprintf(stderr, "NICK %s NOT REGISTERED\n", msg->destination);
                checkout_message(msg_head, msg_list[1]);
            }
            else
            {
                checkout_message(msg_head, msg_list[1]);
            }
            free_split_message(msg_list, msg_size);
        }
        // printf("Checking messages: %i\n", counter);
        check_messages(&msg_head, &user_head, counter, timeout);
        // print_message_count(&msg_head);
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
