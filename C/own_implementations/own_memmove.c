#include <stdio.h>
#include <stddef.h>
#include <string.h>

void *my_memmove(void *dest, const void *src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;

    /*
     * Nothing to copy.
     */
    if (n == 0 || dest == src)
        return dest;

    /*
     * Use glibc-style address comparison.
     *
     * Unsigned subtraction handles either address order:
     * if dest is before src, the subtraction wraps around.
     */
    unsigned long int dstp = (unsigned long int)dest;
    unsigned long int srcp = (unsigned long int)src;

    if (dstp - srcp >= n)
    {
        /*
         * Copy forward.
         */
        for (size_t i = 0; i < n; i++)
            d[i] = s[i];
    }
    else
    {
        /*
         * Copy backward.
         */
        for (size_t i = n; i > 0; i--)
            d[i - 1] = s[i - 1];
    }

    return dest;
}

static void printBuffer(const char *buffer, size_t n)
{
    for (size_t i = 0; i < n; i++)
        printf("%c ", buffer[i]);

    printf("\n");
}

int main(void)
{
    char buffer1[] = "ABCDEFGH";
    char buffer2[] = "ABCDEFGH";
    char buffer3[] = "ABCDEFGH";

    /*
     * ----------------------------------------
     * memmove:
     * destination AFTER source
     * ----------------------------------------
     *
     * Before:
     *
     * A B C D E F G H
     *
     * Move:
     *
     * [A B C D] -> [C D E F]
     *
     * Expected:
     *
     * A B A B C D G H
     */

    my_memmove(buffer1 + 2, buffer1, 4);

    printf("memmove (destination after source):\n");
    printBuffer(buffer1, 8);

    // memmove(buffer1 + 2, buffer1, 4);
    // printBuffer(buffer1, 8);

    /*
     * ----------------------------------------
     * memmove:
     * destination BEFORE source
     * ----------------------------------------
     *
     * Before:
     *
     * A B C D E F G H
     *
     * Move:
     *
     * [C D E F] -> [A B C D]
     *
     * Expected:
     *
     * C D E F E F G H
     */

    my_memmove(buffer2, buffer2 + 2, 4);

    printf("memmove (destination before source):\n");
    printBuffer(buffer2, 8);

    return 0;
}
