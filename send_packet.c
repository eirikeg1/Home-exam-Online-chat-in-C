#include "header.h"

#include "send_packet.h"
#include "string_operations.h"

static float loss_probability = 0.0f;

void set_loss_probability(float x)
{
    loss_probability = x;
    srand48(3.0);
}

ssize_t send_packet(int sock, void *buffer, size_t size, int flags, struct sockaddr *addr, socklen_t addrlen)
{
    float rnd = drand48();

    if (rnd < loss_probability)
    {
        fprintf(stderr, "Randomly dropping a packet\n");
        return size;
    }

    // char **tmp = malloc(0);
    // int *x = split(&tmp, buffer);
    //  if (strcmp(tmp[2], "REG") != 0)
    //   printf("\n\nSENDING %s\n", (char *)buffer);
    // free_split_message(tmp, x);

    return sendto(sock,
                  buffer,
                  size,
                  flags,
                  addr,
                  addrlen);
}
