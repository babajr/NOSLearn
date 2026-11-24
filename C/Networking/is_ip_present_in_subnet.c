#include <stdio.h>
#include <stdint.h>

uint32_t ip_to_uint32(unsigned int a, unsigned int b, unsigned int c, unsigned int d)
{
    return ((uint32_t)a << 24 |
            (uint32_t)b << 16 |
            (uint32_t)c << 8 |
            (uint32_t)d);
}

uint32_t prefix_to_mask(unsigned int prefix)
{
    if (prefix == 0 || prefix > 32)
        return 0;

    return (0xFFFFFFFF << (32 - prefix));
}

uint32_t get_nw(uint32_t ip, uint32_t mask)
{
    return (ip & mask);
}

int is_ip_in_subnet(uint32_t ip, uint32_t nw, uint32_t mask)
{
    return ((ip & mask) == (nw & mask));
}

int main(void)
{
    char *input = "192.168.2.25/24";

    // get the octets and prefix
    unsigned int a, b, c, d, prefix;
    sscanf(input, "%u.%u.%u.%u/%u", &a, &b, &c, &d, &prefix);

    if (a > 255 || b > 255 || c > 255 || d > 255)
        return 1;

    // convert the IP to uint32_t
    uint32_t ip = ip_to_uint32(a, b, c, d);

    // get the mask from prefix
    uint32_t mask = prefix_to_mask(prefix);

    // get the network
    uint32_t nw = get_nw(ip, mask);

    char *ip1 = "192.168.1.0";
    // get the octets
    unsigned int e, f, g, h;
    sscanf(ip1, "%u.%u.%u.%u", &e, &f, &g, &h);

    uint32_t ip2 = ip_to_uint32(e, f, g, h);

    if (is_ip_in_subnet(ip2, nw, mask) == 1)
        printf("YES\n");
    else
        printf("NO\n");

    return 0;
}
