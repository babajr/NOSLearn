#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>

/*
 * ============================================================
 * ETHER TYPES
 * ============================================================
 */

#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_VLAN 0x8100 /* IEEE 802.1Q */
#define ETHERTYPE_QINQ 0x88A8 /* IEEE 802.1ad */

/*
 * ============================================================
 * IP PROTOCOLS
 * ============================================================
 */

#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17

/*
 * ============================================================
 * HEADER LENGTHS
 * ============================================================
 */

#define ETH_HEADER_LEN 14
#define VLAN_TAG_LEN 4
#define IPV4_HEADER_LEN 20
#define TCP_HEADER_LEN 20
#define UDP_HEADER_LEN 8

/*
 * ============================================================
 * L2 : VLAN INFORMATION
 * ============================================================
 *
 * A VLAN tag consists of:
 *
 *      TPID = 2 bytes
 *      TCI  = 2 bytes
 *
 * The next EtherType follows the VLAN tag.
 *
 *
 * Wire format:
 *
 *      +-------------+
 *      | TPID        | 2 bytes
 *      +-------------+
 *      | TCI         | 2 bytes
 *      +-------------+
 *      | Next Type   | 2 bytes
 *      +-------------+
 *
 *
 * IMPORTANT:
 *
 * For the FIRST VLAN tag:
 *
 *      TPID is the Ethernet EtherType field
 *      at byte offset 12.
 *
 * Therefore we do NOT read TPID again
 * from packet + 14.
 */

struct vlan_info
{
    uint16_t tpid;

    uint8_t pcp;
    bool dei;
    uint16_t vlan_id;

    uint16_t encapsulated_ethertype;
};

/*
 * ============================================================
 * L2 : ETHERNET
 * ============================================================
 */

struct ethernet_info
{
    uint8_t dst_mac[6];
    uint8_t src_mac[6];

    /*
     * First EtherType after Source MAC.
     *
     * For this example:
     *
     *     0x88A8
     */
    uint16_t ethertype;

    /*
     * Number of VLAN tags.
     */
    size_t vlan_count;

    /*
     * Maximum:
     *
     *     Outer S-TAG
     *     Inner C-TAG
     */
    struct vlan_info vlan[2];

    /*
     * Final EtherType after VLAN parsing.
     *
     * For our packet:
     *
     *     0x0800
     */
    uint16_t inner_ethertype;

    /*
     * Pointer to L3.
     */
    const uint8_t *payload;

    /*
     * Bytes available to L3.
     */
    size_t payload_len;
};

/*
 * ============================================================
 * L3 : IPv4
 * ============================================================
 *
 * IPv4 options are ignored.
 *
 * Therefore:
 *
 *     IHL must be 5.
 */

struct ipv4_info
{
    uint8_t version;
    uint8_t ihl;

    uint16_t total_len;
    uint16_t identification;

    bool df;
    bool mf;

    uint16_t frag_offset;

    uint8_t ttl;
    uint8_t protocol;

    uint32_t src_ip;
    uint32_t dst_ip;

    const uint8_t *payload;
    size_t payload_len;
};

/*
 * ============================================================
 * L4 : TCP
 * ============================================================
 *
 * TCP options are ignored.
 *
 * Therefore:
 *
 *     Data Offset must be 5.
 */

struct tcp_info
{
    uint16_t src_port;
    uint16_t dst_port;

    uint32_t seq;
    uint32_t ack;

    uint8_t data_offset;

    bool urg;
    bool ack_flag;
    bool psh;
    bool rst;
    bool syn;
    bool fin;

    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;

    const uint8_t *payload;
    size_t payload_len;
};

/*
 * ============================================================
 * L4 : UDP
 * ============================================================
 */

struct udp_info
{
    uint16_t src_port;
    uint16_t dst_port;

    uint16_t length;
    uint16_t checksum;

    const uint8_t *payload;
    size_t payload_len;
};

/*
 * ============================================================
 * BYTE ORDER HELPERS
 * ============================================================
 */

static uint16_t read_be16(const uint8_t *p)
{
    uint16_t value;

    memcpy(&value, p, sizeof(value));

    return ntohs(value);
}

static uint32_t read_be32(const uint8_t *p)
{
    uint32_t value;

    memcpy(&value, p, sizeof(value));

    return ntohl(value);
}

/*
 * ============================================================
 * IS VLAN TPID?
 * ============================================================
 */

static bool is_vlan_ethertype(uint16_t ethertype)
{
    return (ethertype == ETHERTYPE_VLAN ||
            ethertype == ETHERTYPE_QINQ);
}

/*
 * ============================================================
 * PARSE VLAN TAG
 * ============================================================
 *
 * IMPORTANT:
 *
 * 'tpid' is supplied by the caller.
 *
 * 'packet' points to the TCI.
 *
 *
 * Example:
 *
 *      tpid = 0x88A8
 *
 *      packet
 *         │
 *         ▼
 *       60 64       <- TCI
 *       81 00       <- next EtherType
 *
 *
 * Therefore:
 *
 *      packet + 0 -> TCI
 *      packet + 2 -> Next EtherType
 *
 * The VLAN tag itself is:
 *
 *      TCI = 2 bytes
 *
 * plus the TPID that preceded it.
 */

static int parse_vlan(uint16_t tpid,
                      const uint8_t *packet,
                      size_t available,
                      struct vlan_info *info)
{
    /*
     * Need:
     *
     *     TCI             = 2 bytes
     *     Next EtherType  = 2 bytes
     *
     * Total = 4 bytes
     */

    if (available < VLAN_TAG_LEN)
    {
        return -1;
    }

    /*
     * Save TPID supplied by caller.
     */

    info->tpid = tpid;

    /*
     * Read TCI.
     */

    uint16_t tci =
        read_be16(packet + 0);

    /*
     * PCP:
     *
     * bits 15-13
     */

    info->pcp =
        (tci >> 13) & 0x07;

    /*
     * DEI:
     *
     * bit 12
     */

    info->dei =
        (tci & 0x1000) != 0;

    /*
     * VLAN ID:
     *
     * bits 11-0
     */

    info->vlan_id =
        tci & 0x0FFF;

    /*
     * Next EtherType.
     *
     * This can be:
     *
     *     0x8100 -> another VLAN
     *     0x0800 -> IPv4
     *     etc.
     */

    info->encapsulated_ethertype =
        read_be16(packet + 2);

    return 0;
}

/*
 * ============================================================
 * L2 : ETHERNET PARSER
 * ============================================================
 *
 * Supports:
 *
 *     Untagged Ethernet
 *     802.1Q
 *     802.1ad
 *     QinQ
 *
 *
 * Example QinQ:
 *
 *      Ethernet
 *          |
 *          | 0x88A8
 *          ▼
 *       S-TAG
 *          |
 *          | 0x8100
 *          ▼
 *       C-TAG
 *          |
 *          | 0x0800
 *          ▼
 *        IPv4
 */

static int parse_ethernet(const uint8_t *packet,
                          size_t available,
                          struct ethernet_info *info)
{
    /*
     * Minimum Ethernet header:
     *
     *     Destination MAC = 6
     *     Source MAC      = 6
     *     EtherType       = 2
     *
     * Total = 14
     */

    if (available < ETH_HEADER_LEN)
    {
        return -1;
    }

    /*
     * Destination MAC.
     */

    memcpy(info->dst_mac,
           packet + 0,
           6);

    /*
     * Source MAC.
     */

    memcpy(info->src_mac,
           packet + 6,
           6);

    /*
     * EtherType.
     *
     * This is also the TPID for the first VLAN tag
     * if this is a tagged frame.
     */

    uint16_t ethertype =
        read_be16(packet + 12);

    info->ethertype =
        ethertype;

    info->vlan_count =
        0;

    /*
     * --------------------------------------------------------
     * CRITICAL OFFSET
     * --------------------------------------------------------
     *
     * Offset 14 is NOT the first TPID.
     *
     * Offset 12 contains:
     *
     *     0x88A8
     *
     * Offset 14 contains:
     *
     *     TCI = 0x6064
     */

    size_t offset =
        ETH_HEADER_LEN;

    /*
     * --------------------------------------------------------
     * VLAN LOOP
     * --------------------------------------------------------
     */

    while (is_vlan_ethertype(ethertype))
    {

        /*
         * We support maximum two VLAN tags.
         */

        if (info->vlan_count >= 2)
        {
            return -1;
        }

        /*
         * At 'offset' we are pointing to TCI.
         *
         * Need:
         *
         *     TCI + next EtherType
         *
         *     2 + 2 = 4 bytes
         */

        if (available - offset < VLAN_TAG_LEN)
        {
            return -1;
        }

        /*
         * Parse VLAN.
         *
         * Notice:
         *
         *     ethertype = TPID
         *
         *     packet + offset = TCI
         */

        if (parse_vlan(
                ethertype,
                packet + offset,
                available - offset,
                &info->vlan[info->vlan_count]) != 0)
        {

            return -1;
        }

        /*
         * One VLAN successfully parsed.
         */

        info->vlan_count++;

        /*
         * The next EtherType becomes
         * the current EtherType.
         *
         * Example:
         *
         *     First iteration:
         *
         *         88A8 -> 8100
         *
         *     Second iteration:
         *
         *         8100 -> 0800
         */

        ethertype =
            info->vlan[info->vlan_count - 1]
                .encapsulated_ethertype;

        /*
         * Move past:
         *
         *     TCI           = 2 bytes
         *     Next EtherType= 2 bytes
         *
         * Total = 4 bytes.
         *
         * After this:
         *
         *     offset points to the next TCI
         *     if another VLAN exists.
         */

        offset += VLAN_TAG_LEN;
    }

    /*
     * We have reached the final EtherType.
     */

    info->inner_ethertype =
        ethertype;

    /*
     * L3 begins at current offset.
     */

    info->payload =
        packet + offset;

    info->payload_len =
        available - offset;

    return 0;
}

/*
 * ============================================================
 * L3 : IPv4 PARSER
 * ============================================================
 */

static int parse_ipv4(const uint8_t *ip,
                      size_t available,
                      struct ipv4_info *info)
{
    if (available < IPV4_HEADER_LEN)
    {
        return -1;
    }

    /*
     * Version / IHL.
     */

    uint8_t version =
        ip[0] >> 4;

    uint8_t ihl =
        ip[0] & 0x0F;

    if (version != 4)
    {
        return -1;
    }

    /*
     * IPv4 options ignored.
     */

    if (ihl != 5)
    {
        return -1;
    }

    /*
     * Header = 20 bytes.
     */

    uint16_t total_len =
        read_be16(ip + 2);

    if (total_len < IPV4_HEADER_LEN)
    {
        return -1;
    }

    if (total_len > available)
    {
        return -1;
    }

    uint16_t identification =
        read_be16(ip + 4);

    uint16_t flags_frag =
        read_be16(ip + 6);

    bool df =
        (flags_frag & 0x4000) != 0;

    bool mf =
        (flags_frag & 0x2000) != 0;

    uint16_t frag_offset =
        flags_frag & 0x1FFF;

    info->version =
        version;

    info->ihl =
        ihl;

    info->total_len =
        total_len;

    info->identification =
        identification;

    info->df =
        df;

    info->mf =
        mf;

    info->frag_offset =
        frag_offset;

    info->ttl =
        ip[8];

    info->protocol =
        ip[9];

    info->src_ip =
        read_be32(ip + 12);

    info->dst_ip =
        read_be32(ip + 16);

    /*
     * L4 starts after IPv4 header.
     */

    info->payload =
        ip + IPV4_HEADER_LEN;

    info->payload_len =
        total_len - IPV4_HEADER_LEN;

    return 0;
}

/*
 * ============================================================
 * L4 : TCP PARSER
 * ============================================================
 */

static int parse_tcp(const uint8_t *tcp,
                     size_t available,
                     struct tcp_info *info)
{
    if (available < TCP_HEADER_LEN)
    {
        return -1;
    }

    info->src_port =
        read_be16(tcp + 0);

    info->dst_port =
        read_be16(tcp + 2);

    info->seq =
        read_be32(tcp + 4);

    info->ack =
        read_be32(tcp + 8);

    /*
     * Data Offset.
     */

    uint8_t data_offset =
        tcp[12] >> 4;

    /*
     * TCP options ignored.
     */

    if (data_offset != 5)
    {
        return -1;
    }

    info->data_offset =
        data_offset;

    /*
     * TCP flags.
     */

    uint16_t flags =
        read_be16(tcp + 12);

    info->fin =
        (flags & 0x0001) != 0;

    info->syn =
        (flags & 0x0002) != 0;

    info->rst =
        (flags & 0x0004) != 0;

    info->psh =
        (flags & 0x0008) != 0;

    info->ack_flag =
        (flags & 0x0010) != 0;

    info->urg =
        (flags & 0x0020) != 0;

    info->window =
        read_be16(tcp + 14);

    info->checksum =
        read_be16(tcp + 16);

    info->urgent_ptr =
        read_be16(tcp + 18);

    /*
     * TCP payload.
     */

    info->payload =
        tcp + TCP_HEADER_LEN;

    info->payload_len =
        available - TCP_HEADER_LEN;

    return 0;
}

/*
 * ============================================================
 * L4 : UDP PARSER
 * ============================================================
 */

static int parse_udp(const uint8_t *udp,
                     size_t available,
                     struct udp_info *info)
{
    if (available < UDP_HEADER_LEN)
    {
        return -1;
    }

    info->src_port =
        read_be16(udp + 0);

    info->dst_port =
        read_be16(udp + 2);

    info->length =
        read_be16(udp + 4);

    info->checksum =
        read_be16(udp + 6);

    /*
     * UDP length includes UDP header.
     */

    if (info->length < UDP_HEADER_LEN)
    {
        return -1;
    }

    if (info->length > available)
    {
        return -1;
    }

    info->payload =
        udp + UDP_HEADER_LEN;

    info->payload_len =
        info->length - UDP_HEADER_LEN;

    return 0;
}

/*
 * ============================================================
 * PRINT HELPERS
 * ============================================================
 */

static void print_mac(const uint8_t *mac)
{
    printf("%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0],
           mac[1],
           mac[2],
           mac[3],
           mac[4],
           mac[5]);
}

// static void print_ipv4(uint32_t ip)
// {
//     struct in_addr addr;

//     addr.s_addr =
//         htonl(ip);

//     char buffer[INET_ADDRSTRLEN];

//     if (inet_ntop(AF_INET,
//                   &addr,
//                   buffer,
//                   sizeof(buffer)) != NULL)
//     {

//         printf("%s", buffer);
//     }
// }

static void print_ipv4(uint32_t ip)
{
    printf("%u.%u.%u.%u",
           (ip >> 24) & 0xFF,
           (ip >> 16) & 0xFF,
           (ip >> 8) & 0xFF,
           ip & 0xFF);
}

static void print_payload(const uint8_t *payload,
                          size_t length)
{
    printf("Payload (%zu bytes): ",
           length);

    for (size_t i = 0; i < length; i++)
    {
        printf("%02x ", payload[i]);
    }

    printf("\n");
}

static void print_vlan(const struct vlan_info *vlan,
                       size_t index)
{
    printf("\n");
    printf("VLAN[%zu]\n", index);

    printf("  TPID       : 0x%04x\n",
           vlan->tpid);

    printf("  PCP        : %u\n",
           vlan->pcp);

    printf("  DEI        : %s\n",
           vlan->dei ? "YES" : "NO");

    printf("  VLAN ID    : %u\n",
           vlan->vlan_id);

    printf("  Next Type  : 0x%04x\n",
           vlan->encapsulated_ethertype);
}

/*
 * ============================================================
 * MAIN
 * ============================================================
 */

int main(void)
{
    /*
     * ========================================================
     * PACKET
     * ========================================================
     *
     * Ethernet
     *     |
     *     | 0x88A8
     *     ▼
     * S-TAG
     *     |
     *     | 0x8100
     *     ▼
     * C-TAG
     *     |
     *     | 0x0800
     *     ▼
     * IPv4
     *     |
     *     | Protocol = TCP
     *     ▼
     * TCP
     *     |
     *     ▼
     * HELLO
     *
     *
     * Total:
     *
     * Ethernet = 14
     * S-TAG    = 4
     * C-TAG    = 4
     * IPv4     = 20
     * TCP      = 20
     * Payload  = 5
     *
     * Total = 67
     */

    uint8_t packet[67] = {

        /*
         * ====================================================
         * Ethernet
         * ====================================================
         */

        /* Destination MAC */

        0x00, 0x11, 0x22,
        0x33, 0x44, 0x55,

        /* Source MAC */

        0x66, 0x77, 0x88,
        0x99, 0xaa, 0xbb,

        /*
         * EtherType / Outer TPID
         *
         * 802.1ad
         */

        0x88, 0xA8,

        /*
         * ====================================================
         * OUTER S-TAG TCI
         * ====================================================
         *
         * PCP = 3
         * DEI = 0
         * VID = 100
         *
         * TCI = 0x6064
         */

        0x60, 0x64,

        /*
         * Next EtherType / Inner TPID
         *
         * 802.1Q
         */

        0x81, 0x00,

        /*
         * ====================================================
         * INNER C-TAG TCI
         * ====================================================
         *
         * PCP = 5
         * DEI = 0
         * VID = 200
         *
         * TCI = 0xA0C8
         */

        0xA0, 0xC8,

        /*
         * Next EtherType
         *
         * IPv4
         */

        0x08, 0x00,

        /*
         * ====================================================
         * IPv4
         * ====================================================
         */

        /* Version 4, IHL 5 */

        0x45,

        /* DSCP / ECN */

        0x00,

        /*
         * Total Length = 45
         */

        0x00, 0x2D,

        /* Identification */

        0x12, 0x34,

        /* DF */

        0x40, 0x00,

        /* TTL */

        0x40,

        /* Protocol = TCP */

        0x06,

        /* IPv4 checksum */

        0x00, 0x00,

        /* Source = 192.168.1.10 */

        0xC0, 0xA8, 0x01, 0x0A,

        /* Destination = 192.168.1.20 */

        0xC0, 0xA8, 0x01, 0x14,

        /*
         * ====================================================
         * TCP
         * ====================================================
         */

        /* Source port = 12345 */

        0x30, 0x39,

        /* Destination port = 80 */

        0x00, 0x50,

        /* Sequence = 1000 */

        0x00, 0x00, 0x03, 0xE8,

        /* ACK = 2000 */

        0x00, 0x00, 0x07, 0xD0,

        /* Data Offset = 5 */

        0x50,

        /* ACK */

        0x10,

        /* Window = 65535 */

        0xFF, 0xFF,

        /* TCP checksum */

        0x00, 0x00,

        /* Urgent pointer */

        0x00, 0x00,

        /*
         * ====================================================
         * APPLICATION DATA
         * ====================================================
         */

        'H',
        'E',
        'L',
        'L',
        'O'};

    size_t packet_len =
        sizeof(packet);

    /*
     * ========================================================
     * L2
     * ========================================================
     */

    struct ethernet_info eth;

    if (parse_ethernet(packet,
                       packet_len,
                       &eth) != 0)
    {

        printf("Ethernet parsing failed\n");

        return 1;
    }

    printf("========================================\n");
    printf("L2 : Ethernet\n");
    printf("========================================\n");

    printf("Destination MAC : ");
    print_mac(eth.dst_mac);
    printf("\n");

    printf("Source MAC      : ");
    print_mac(eth.src_mac);
    printf("\n");

    printf("First EtherType : 0x%04x\n",
           eth.ethertype);

    printf("VLAN Count      : %zu\n",
           eth.vlan_count);

    for (size_t i = 0;
         i < eth.vlan_count;
         i++)
    {

        print_vlan(&eth.vlan[i], i);
    }

    printf("\n");

    printf("Inner EtherType : 0x%04x\n",
           eth.inner_ethertype);

    printf("L3 Payload      : %zu bytes\n",
           eth.payload_len);

    /*
     * ========================================================
     * L3 DISPATCH
     * ========================================================
     */

    if (eth.inner_ethertype != ETHERTYPE_IPV4)
    {

        printf("Not an IPv4 packet\n");

        return 0;
    }

    /*
     * ========================================================
     * IPv4
     * ========================================================
     */

    struct ipv4_info ip;

    if (parse_ipv4(eth.payload,
                   eth.payload_len,
                   &ip) != 0)
    {

        printf("IPv4 parsing failed\n");

        return 1;
    }

    printf("\n");
    printf("========================================\n");
    printf("L3 : IPv4\n");
    printf("========================================\n");

    printf("Version         : %u\n",
           ip.version);

    printf("IHL             : %u (%u bytes)\n",
           ip.ihl,
           ip.ihl * 4);

    printf("Total Length    : %u\n",
           ip.total_len);

    printf("Identification  : 0x%04x\n",
           ip.identification);

    printf("DF              : %s\n",
           ip.df ? "YES" : "NO");

    printf("MF              : %s\n",
           ip.mf ? "YES" : "NO");

    printf("Fragment Offset : %u\n",
           ip.frag_offset);

    printf("TTL             : %u\n",
           ip.ttl);

    printf("Protocol        : %u\n",
           ip.protocol);

    printf("Source IP       : ");
    print_ipv4(ip.src_ip);
    printf("\n");

    printf("Destination IP  : ");
    print_ipv4(ip.dst_ip);
    printf("\n");

    printf("L4 Payload      : %zu bytes\n",
           ip.payload_len);

    /*
     * ========================================================
     * L4
     * ========================================================
     */

    if (ip.protocol == IP_PROTO_TCP)
    {

        struct tcp_info tcp;

        if (parse_tcp(ip.payload,
                      ip.payload_len,
                      &tcp) != 0)
        {

            printf("TCP parsing failed\n");

            return 1;
        }

        printf("\n");
        printf("========================================\n");
        printf("L4 : TCP\n");
        printf("========================================\n");

        printf("Source Port      : %u\n",
               tcp.src_port);

        printf("Destination Port : %u\n",
               tcp.dst_port);

        printf("Sequence Number  : %u\n",
               tcp.seq);

        printf("ACK Number       : %u\n",
               tcp.ack);

        printf("Data Offset      : %u (%u bytes)\n",
               tcp.data_offset,
               tcp.data_offset * 4);

        printf("FIN              : %s\n",
               tcp.fin ? "YES" : "NO");

        printf("SYN              : %s\n",
               tcp.syn ? "YES" : "NO");

        printf("RST              : %s\n",
               tcp.rst ? "YES" : "NO");

        printf("PSH              : %s\n",
               tcp.psh ? "YES" : "NO");

        printf("ACK              : %s\n",
               tcp.ack_flag ? "YES" : "NO");

        printf("URG              : %s\n",
               tcp.urg ? "YES" : "NO");

        printf("Window           : %u\n",
               tcp.window);

        printf("Checksum         : 0x%04x\n",
               tcp.checksum);

        printf("Urgent Pointer   : %u\n",
               tcp.urgent_ptr);

        print_payload(tcp.payload,
                      tcp.payload_len);
    }
    else if (ip.protocol == IP_PROTO_UDP)
    {

        struct udp_info udp;

        if (parse_udp(ip.payload,
                      ip.payload_len,
                      &udp) != 0)
        {

            printf("UDP parsing failed\n");

            return 1;
        }

        printf("\n");
        printf("========================================\n");
        printf("L4 : UDP\n");
        printf("========================================\n");

        printf("Source Port      : %u\n",
               udp.src_port);

        printf("Destination Port : %u\n",
               udp.dst_port);

        printf("Length           : %u\n",
               udp.length);

        printf("Checksum         : 0x%04x\n",
               udp.checksum);

        print_payload(udp.payload,
                      udp.payload_len);
    }
    else if (ip.protocol == IP_PROTO_ICMP)
    {

        printf("\n");
        printf("L4 : ICMP\n");
    }
    else
    {

        printf("\n");
        printf("Unknown L4 protocol: %u\n",
               ip.protocol);
    }

    return 0;
}
