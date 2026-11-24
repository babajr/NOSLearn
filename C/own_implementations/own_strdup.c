#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *own_strdup(char *src)
{
    if (src == NULL)
        return NULL;

    int len = strlen(src);

#if 0 /* OVERFLOW CHECK */    
    if (len == SIZE_MAX)
        return NULL;
#endif
    char *dst = (char *)malloc(sizeof(char) * (len + 1));

    if (dst == NULL)
        return NULL;

    memcpy(dst, src, (len + 1));

    return dst;
}

int main(void)
{
    char *src = "hello";
    char *dst = NULL;

    // dst = strdup(src);
    // printf("src:%s dst:%s\n", src, dst);

    dst = own_strdup(src);
    printf("src:%s dst:%s\n", src, dst);

    free(dst);

    return 0;
}
