#include <stdio.h>
#include <string.h>
#include <stdint.h>

uint32_t ip_to_uint32(unsigned int a, unsigned int b, unsigned int c, unsigned int d)
{
    return ((uint32_t)a << 24) |
           ((uint32_t)b << 16) |
           ((uint32_t)c << 8) |
           ((uint32_t)d);
}

uint32_t get_mask(unsigned int prefix)
{
    if (prefix > 32 || prefix == 0)
        return 0;

    return (0xFFFFFFFF << (32 - prefix));
}

uint32_t calculate_nw(uint32_t ip, uint32_t mask)
{
    return (ip & mask);
}

void print_uint32(uint32_t ip)
{
    printf("%u.%u.%u.%u\n",
           ((ip >> 24) & 0xFF),
           ((ip >> 16) & 0xFF),
           ((ip >> 8) & 0xFF),
           (ip & 0xFF));
}

int main(void)
{
    char *input = "192.168.10.1/24";

    // Parse the input and get the octets as well as prefix and validate
    unsigned int a, b, c, d, prefix;
    sscanf(input, "%u.%u.%u.%u/%u", &a, &b, &c, &d, &prefix);

    if (a > 255 || b > 255 || c > 255 || d > 255)
        return 1;

    // Convert IP address to uint32_t
    uint32_t ip = ip_to_uint32(a, b, c, d);

    // Get the mask from the prefix
    uint32_t mask = get_mask(prefix);

    // Calculate the network from the IP and mask
    uint32_t network = calculate_nw(ip, mask);

    // Helper to print the network address
    print_uint32(network);

    return 0;
}
