#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

struct route
{
    uint32_t network;
    uint32_t mask;
    uint8_t prefix_len;
};

static uint32_t prefix_to_mask(uint8_t prefix_len)
{
    if (prefix_len == 0)
    {
        return 0;
    }

    return 0xFFFFFFFFu << (32 - prefix_len);
}

static int parse_ipv4(const char *str, uint32_t *ip)
{
    unsigned int a, b, c, d;

    if (sscanf(str, "%u.%u.%u.%u",
               &a, &b, &c, &d) != 4)
    {
        return -1;
    }

    if (a > 255 || b > 255 ||
        c > 255 || d > 255)
    {
        return -1;
    }

    *ip = ((uint32_t)a << 24) |
          ((uint32_t)b << 16) |
          ((uint32_t)c << 8) |
          (uint32_t)d;

    return 0;
}

static const struct route *
longest_prefix_match(uint32_t destination,
                     const struct route *routes,
                     size_t route_count)
{
    const struct route *best = NULL;

    for (size_t i = 0; i < route_count; i++)
    {

        uint32_t candidate_network =
            destination & routes[i].mask;

        if (candidate_network != routes[i].network)
        {
            continue;
        }

        if (best == NULL ||
            routes[i].prefix_len > best->prefix_len)
        {
            best = &routes[i];
        }
    }

    return best;
}

static void print_ipv4(uint32_t ip)
{
    printf("%u.%u.%u.%u",
           (ip >> 24) & 0xFF,
           (ip >> 16) & 0xFF,
           (ip >> 8) & 0xFF,
           ip & 0xFF);
}

int main(void)
{
    const char *destination_str = "10.1.1.77";

    uint32_t destination;

    if (parse_ipv4(destination_str, &destination) != 0)
    {
        printf("Invalid IP address\n");
        return 1;
    }

    struct route routes[] = {
        {.network = 0x00000000,
         .mask = prefix_to_mask(0),
         .prefix_len = 0},
        {.network = 0x0A000000,
         .mask = prefix_to_mask(8),
         .prefix_len = 8},
        {.network = 0x0A010000,
         .mask = prefix_to_mask(16),
         .prefix_len = 16},
        {.network = 0x0A010100,
         .mask = prefix_to_mask(24),
         .prefix_len = 24}};

    size_t route_count =
        sizeof(routes) / sizeof(routes[0]);

    const struct route *best =
        longest_prefix_match(destination,
                             routes,
                             route_count);

    printf("Destination: ");
    print_ipv4(destination);
    printf("\n");

    if (best == NULL)
    {
        printf("No matching route\n");
        return 0;
    }

    printf("Best route: ");
    print_ipv4(best->network);
    printf("/%u\n", best->prefix_len);

    return 0;
}
