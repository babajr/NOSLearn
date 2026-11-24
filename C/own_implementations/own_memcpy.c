#include <stdio.h>
#include <stddef.h>

void *own_memcpy(void *dst, void *src, size_t n)
{
    if (dst == NULL || src == NULL)
        return dst;

    unsigned char *d = dst;
    unsigned char *s = src;

    for (size_t i = 0; i < n; i++)
        d[i] = s[i];

    return dst;
}

int main(void)
{
    char src_str[] = "Hello, hi";
    char dst_str[20];

    own_memcpy(dst_str, src_str, sizeof(src_str));
    printf("src_str:%s\ndst_str:%s\n", src_str, dst_str);

    return 0;
}
