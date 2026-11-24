#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_BITS 32

/*
 * One node in the binary Trie.
 *
 * child[0] -> next node if the current bit is 0
 * child[1] -> next node if the current bit is 1
 *
 * is_route -> this node represents a valid routing prefix
 */
struct LpmNode
{
    struct LpmNode *child[2];

    bool is_route;

    uint8_t prefix_len;
    uint32_t next_hop;
};

struct LpmTrie
{
    struct LpmNode *root;
};

/*
 * Create one Trie node.
 */
static struct LpmNode *lpmNodeCreate(void)
{
    struct LpmNode *node;

    node = calloc(1, sizeof(*node));

    return node;
}

/*
 * Create an empty LPM Trie.
 */
static struct LpmTrie *lpmTrieCreate(void)
{
    struct LpmTrie *trie;

    trie = malloc(sizeof(*trie));

    if (trie == NULL)
        return NULL;

    trie->root = lpmNodeCreate();

    if (trie->root == NULL)
    {
        free(trie);
        return NULL;
    }

    return trie;
}

/*
 * Insert a route into the Trie.
 *
 * Example:
 *
 *     10.1.1.0/24
 *
 * Only the first 24 bits are inserted.
 */
static bool lpmInsert(struct LpmTrie *trie,
                      uint32_t network,
                      uint8_t prefix_len,
                      uint32_t next_hop)
{
    struct LpmNode *node;
    int bit_pos;

    if (trie == NULL || prefix_len > MAX_BITS)
        return false;

    node = trie->root;

    /*
     * Process only the bits belonging to the prefix.
     *
     * /8  -> bits 31 ... 24
     * /16 -> bits 31 ... 16
     * /24 -> bits 31 ... 8
     * /32 -> bits 31 ... 0
     */
    for (bit_pos = 31;
         bit_pos >= 32 - prefix_len;
         bit_pos--)
    {

        int bit;

        /*
         * Extract the current bit.
         */
        bit = (network >> bit_pos) & 1U;

        /*
         * If this path does not exist,
         * create the child node.
         */
        if (node->child[bit] == NULL)
        {

            node->child[bit] = lpmNodeCreate();

            if (node->child[bit] == NULL)
                return false;
        }

        /*
         * Move to the node representing
         * the prefix including this bit.
         */
        node = node->child[bit];
    }

    /*
     * We have reached the node representing
     * the complete prefix.
     *
     * Mark it as a routing prefix.
     */
    node->is_route = true;
    node->prefix_len = prefix_len;
    node->next_hop = next_hop;

    return true;
}

/*
 * Perform Longest Prefix Match.
 *
 * Given a destination IP, walk the Trie
 * according to its bits from MSB to LSB.
 *
 * 'best' always remembers the deepest
 * routing prefix encountered so far.
 */
static struct LpmNode *
lpmLookup(const struct LpmTrie *trie,
          uint32_t destination)
{
    struct LpmNode *node;
    struct LpmNode *best;
    int bit_pos;

    if (trie == NULL || trie->root == NULL)
        return NULL;

    node = trie->root;
    best = NULL;

    /*
     * The root represents the /0 prefix.
     *
     * If a default route exists, it is
     * initially our best match.
     */
    if (node->is_route)
        best = node;

    /*
     * Process destination bits from
     * most significant bit to least
     * significant bit.
     */
    for (bit_pos = 31; bit_pos >= 0; bit_pos--)
    {

        int bit;

        /*
         * Extract the current destination bit.
         */
        bit = (destination >> bit_pos) & 1U;

        /*
         * There is no path corresponding
         * to this destination bit.
         *
         * Therefore no longer prefix can
         * possibly match.
         */
        if (node->child[bit] == NULL)
            break;

        /*
         * Move to the child representing
         * the prefix including this bit.
         */
        node = node->child[bit];

        /*
         * If this node represents a route,
         * remember it.
         *
         * Because we are moving deeper into
         * the Trie, this route is more specific
         * than any previously found route.
         */
        if (node->is_route)
            best = node;
    }

    return best;
}

/*
 * Recursively free the Trie nodes.
 */
static void lpmNodeFree(struct LpmNode *node)
{
    if (node == NULL)
        return;

    lpmNodeFree(node->child[0]);
    lpmNodeFree(node->child[1]);

    free(node);
}

/*
 * Free the complete Trie.
 */
static void lpmTrieFree(struct LpmTrie *trie)
{
    if (trie == NULL)
        return;

    lpmNodeFree(trie->root);

    free(trie);
}

/*
 * Convert dotted-decimal IPv4 string
 * into our uint32_t representation.
 *
 * Example:
 *
 *     10.1.1.77
 *
 * becomes:
 *
 *     0x0A01014D
 */
static bool parseIPv4(const char *str, uint32_t *ip)
{
    unsigned int a;
    unsigned int b;
    unsigned int c;
    unsigned int d;

    if (str == NULL || ip == NULL)
        return false;

    if (sscanf(str, "%u.%u.%u.%u",
               &a, &b, &c, &d) != 4)
        return false;

    if (a > 255 ||
        b > 255 ||
        c > 255 ||
        d > 255)
        return false;

    *ip = ((uint32_t)a << 24) |
          ((uint32_t)b << 16) |
          ((uint32_t)c << 8) |
          (uint32_t)d;

    return true;
}

/*
 * Print a uint32_t IPv4 address
 * in dotted-decimal notation.
 */
static void printIPv4(uint32_t ip)
{
    printf("%u.%u.%u.%u",
           (ip >> 24) & 0xFF,
           (ip >> 16) & 0xFF,
           (ip >> 8) & 0xFF,
           ip & 0xFF);
}

int main(void)
{
    struct LpmTrie *trie;

    uint32_t network;
    uint32_t destination;

    struct LpmNode *result;

    /*
     * Create Trie.
     */
    trie = lpmTrieCreate();

    if (trie == NULL)
    {
        fprintf(stderr, "Failed to create Trie\n");
        return EXIT_FAILURE;
    }

    /*
     * ------------------------------------------------
     * Insert routes
     * ------------------------------------------------
     */

    /*
     * Default route:
     *
     * 0.0.0.0/0
     *
     * next_hop = 1
     */
    if (!parseIPv4("0.0.0.0", &network) ||
        !lpmInsert(trie, network, 0, 1))
    {

        fprintf(stderr, "Failed to insert 0.0.0.0/0\n");

        lpmTrieFree(trie);
        return EXIT_FAILURE;
    }

    /*
     * 10.0.0.0/8
     *
     * next_hop = 2
     */
    if (!parseIPv4("10.0.0.0", &network) ||
        !lpmInsert(trie, network, 8, 2))
    {

        fprintf(stderr, "Failed to insert 10.0.0.0/8\n");

        lpmTrieFree(trie);
        return EXIT_FAILURE;
    }

    /*
     * 10.1.0.0/16
     *
     * next_hop = 3
     */
    if (!parseIPv4("10.1.0.0", &network) ||
        !lpmInsert(trie, network, 16, 3))
    {

        fprintf(stderr, "Failed to insert 10.1.0.0/16\n");

        lpmTrieFree(trie);
        return EXIT_FAILURE;
    }

    /*
     * 10.1.1.0/24
     *
     * next_hop = 4
     */
    if (!parseIPv4("10.1.1.0", &network) ||
        !lpmInsert(trie, network, 24, 4))
    {

        fprintf(stderr, "Failed to insert 10.1.1.0/24\n");

        lpmTrieFree(trie);
        return EXIT_FAILURE;
    }

    /*
     * ------------------------------------------------
     * Perform lookup
     * ------------------------------------------------
     */

    /*
     * Destination:
     *
     * 10.1.1.77
     */
    if (!parseIPv4("10.1.1.77", &destination))
    {

        fprintf(stderr, "Invalid destination IP\n");

        lpmTrieFree(trie);
        return EXIT_FAILURE;
    }

    result = lpmLookup(trie, destination);

    /*
     * ------------------------------------------------
     * Print result
     * ------------------------------------------------
     */

    printf("Destination : ");
    printIPv4(destination);
    printf("\n");

    if (result == NULL)
    {

        printf("LPM         : No route\n");
    }
    else
    {

        printf("LPM         : /%u\n",
               result->prefix_len);

        printf("Next hop    : %u\n",
               result->next_hop);
    }

    /*
     * Free everything.
     */
    lpmTrieFree(trie);

    return EXIT_SUCCESS;
}
