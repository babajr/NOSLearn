/*
PROTOTYPE:
int atoi(const char *str);
It converts a string characters to an integer value.
*/

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/*
APPROACH 1:
This implementation handles the negative numbers.
If the first character is ‘-‘ then store the sign as negative and then convert the rest
of the string to number using the previous approach while multiplying sign with it.
*/

// A simple atoi() function
int my_atoi_1(char *str)
{
    // Initialize result
    int res = 0;

    // Initialize sign as positive
    int sign = 1;

    // Initialize index of first digit
    int i = 0;

    // If number is negative,
    // then update sign
    if (str[0] == '-')
    {
        sign = -1;

        // Also update index of first digit
        i++;
    }

    // Iterate through all digits
    // and update the result
    for (; str[i] != '\0'; i++)
    {
        if (str[i] < '0' || str[i] > '9')
            return -1;
        res = res * 10 + str[i] - '0';
    }
    // Return result with sign
    return sign * res;
}

/*
APPROACH 2: Considering all the corner cases

Four corner cases needs to be handled:
- Skip leading spaces
- Sign of the number
- Skip leading zeros
- Process each digit using res = res * 10 + digit.
- Before each update, check whether the new digit would cause overflow
  (res * 10 + digit) > limit --> Overflow
  res > (limit - digit) / 10 --> Rearranged above statement.
- Apply the sign, handling INT_MIN separately.

To remove the leading whitespaces run a loop until a character of the digit is reached.
If the number is greater than or equal to INT_MAX/10.
Then return INT_MAX if the sign is positive and return INT_MIN if the sign is negative.
The other cases are handled in previous approaches.
*/

int my_atoi(const char *str)
{
    int sign = 1;
    unsigned int res = 0;
    int i = 0;
    int len = strlen(str);

    /* Ignore leading spaces */
    while (i < len && str[i] == ' ')
        i++;

    /* Check the sign */
    if (i < len && (str[i] == '-' || str[i] == '+'))
    {
        if (str[i] == '-')
            sign = -1;

        i++;
    }

    /* No digits after the optional sign */
    if (i == len)
        return -1;

    /* Ignore leading zeros */
    while (i < len && str[i] == '0')
        i++;

    /* The input contains only zeros */
    if (i == len)
        return 0;

    /* Set the maximum allowed magnitude */
    unsigned int limit = (unsigned int)INT_MAX;

    if (sign == -1)
        limit++;

    /* Convert digits to integer */
    while (i < len)
    {
        unsigned int digit;

        if (str[i] < '0' || str[i] > '9')
            break; // return -1;

        digit = (unsigned int)(str[i] - '0');

        /* Check overflow before calculating the next value */
        if (res > (limit - digit) / 10U)
            return (sign == -1) ? INT_MIN : INT_MAX;

        res = res * 10U + digit;
        i++;
    }

    /* Apply the sign safely */
    if (sign == -1)
    {
        if (res == (unsigned int)INT_MAX + 1U)
            return INT_MIN;

        return -(int)res;
    }

    return (int)res;
}

int my_atoi_bit_man(char *p)
{
    int k = 0;
    int sign = 1;
    while (*p)
    {
        if (*p == ' ' || *p == '0')
        {
            p++;
        }

        if (*p == '-')
        {
            sign = -1;
            p++;
        }

        if (*p > '9' || *p < '0')
        {
            return -1;
        }

        k = (k << 3) + (k << 1) + (*p) - '0';
        p++;
    }

    return (k * sign);
}

int main(void)
{
    // char str[20] = "12a34";
    char str[20] = "-12";

    // printf("%d\n", my_atoi_1(str));
    printf("%d\n", my_atoi(str));
    printf("%d\n", atoi(str));
    // printf("%d\n", my_atoi_bit_man(str));

    return 0;
}
