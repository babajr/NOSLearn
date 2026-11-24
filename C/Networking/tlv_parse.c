#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * Wire format:
 *
 *   +------+----------------+-------------------+
 *   | TYPE | LENGTH (2 B)   | VALUE (LENGTH B)  |
 *   +------+----------------+-------------------+
 *
 * TYPE:   1 byte
 * LENGTH: 2 bytes, unsigned big-endian
 * VALUE:  LENGTH bytes
 */

#define TLV_HEADER_SIZE ((size_t)3)
#define TLV_MAX_VALUE_LENGTH ((size_t)4096)

typedef enum
{
    TLV_STATUS_OK = 0,      /* One TLV was successfully parsed. */
    TLV_STATUS_END,         /* No bytes remain at a record boundary. */
    TLV_STATUS_NEED_MORE,   /* Current input is incomplete; more may arrive. */
    TLV_STATUS_INVALID,     /* Malformed or truncated final input. */
    TLV_STATUS_TOO_LARGE,   /* Value length exceeds configured maximum. */
    TLV_STATUS_BAD_ARGUMENT /* Invalid function arguments. */
} tlv_status_t;

/*
 * A non-owning view into the caller's input buffer.
 *
 * value points into the original buffer.
 * The parser does not allocate memory or copy the value.
 */
typedef struct
{
    uint8_t type;
    uint16_t length;
    const uint8_t *value;
} tlv_view_t;

/*
 * Parse exactly one TLV record.
 *
 * buffer:
 *   Input byte buffer.
 *
 * buffer_len:
 *   Number of valid bytes in buffer.
 *
 * offset:
 *   On entry, offset of the next record.
 *   On successful parse, advances to the byte after this TLV.
 *   On failure, remains unchanged.
 *
 * final_input:
 *   true  -> no more bytes will be appended to this input message.
 *   false -> more bytes may arrive later.
 *
 * out:
 *   Receives the parsed TLV view on success.
 *
 * Returns:
 *   TLV_STATUS_OK, END, NEED_MORE, INVALID, TOO_LARGE,
 *   or BAD_ARGUMENT.
 */
tlv_status_t
tlv_parse_next(const uint8_t *buffer,
               size_t buffer_len,
               size_t *offset,
               bool final_input,
               tlv_view_t *out)
{
    size_t remaining;
    size_t value_length;
    size_t value_available;
    size_t record_size;
    size_t current_offset;

    /*
     * Validate arguments before dereferencing them.
     *
     * A NULL buffer is allowed only when buffer_len == 0.
     * This lets callers represent an empty input safely.
     */
    if (offset == NULL || out == NULL)
    {
        return TLV_STATUS_BAD_ARGUMENT;
    }

    if (buffer == NULL && buffer_len != 0)
    {
        return TLV_STATUS_BAD_ARGUMENT;
    }

    current_offset = *offset;

    /*
     * An offset beyond the buffer is always invalid.
     */
    if (current_offset > buffer_len)
    {
        return TLV_STATUS_BAD_ARGUMENT;
    }

    /*
     * Calculate bytes remaining only after proving offset <= buffer_len.
     */
    remaining = buffer_len - current_offset;

    /*
     * There is no next record at this position.
     *
     * END means the caller is currently at a record boundary.
     * For a streaming parser, the caller may append more bytes and call
     * again with the same offset.
     */
    if (remaining == 0)
    {
        return TLV_STATUS_END;
    }

    /*
     * Step 1: Ensure the complete fixed-size header is available.
     */
    if (remaining < TLV_HEADER_SIZE)
    {
        return final_input ? TLV_STATUS_INVALID
                           : TLV_STATUS_NEED_MORE;
    }

    /*
     * Step 2: Decode the header.
     *
     * Do not cast buffer + offset to a packed struct:
     * - the pointer might be unaligned;
     * - structure padding/layout is compiler-dependent;
     * - the wire format has explicit byte order.
     */
    const uint8_t *header = buffer + current_offset;

    uint8_t type = header[0];

    /*
     * LENGTH is unsigned 16-bit big-endian:
     *
     *   length = (high_byte * 256) + low_byte
     *
     * Convert each byte to uint16_t before shifting.
     */
    uint16_t decoded_length =
        (uint16_t)(((uint16_t)header[1] << 8) |
                   (uint16_t)header[2]);

    value_length = (size_t)decoded_length;

    /*
     * Step 3: Enforce the protocol/library limit.
     */
    if (value_length > TLV_MAX_VALUE_LENGTH)
    {
        return TLV_STATUS_TOO_LARGE;
    }

    /*
     * Step 4: Check whether the full value is available.
     *
     * We already proved:
     *   remaining >= TLV_HEADER_SIZE
     *
     * Therefore this subtraction is safe.
     */
    value_available = remaining - TLV_HEADER_SIZE;

    if (value_length > value_available)
    {
        return final_input ? TLV_STATUS_INVALID
                           : TLV_STATUS_NEED_MORE;
    }

    /*
     * Step 5: The entire record is available.
     *
     * The value pointer is valid because:
     * - the header is in bounds;
     * - value_length <= value_available.
     */
    record_size = TLV_HEADER_SIZE + value_length;

    /*
     * Publish the output only after all validation succeeds.
     */
    out->type = type;
    out->length = decoded_length;
    out->value = header + TLV_HEADER_SIZE;

    /*
     * Advance the cursor only on success.
     *
     * This is important: on NEED_MORE or an error, the caller can retry
     * using the same offset after receiving more data or handling the error.
     */
    *offset = current_offset + record_size;

    return TLV_STATUS_OK;
}

/*
 * Parse a complete buffer containing zero or more consecutive TLVs.
 *
 * This wrapper uses final_input = true because the entire buffer is
 * considered to be the complete message.
 *
 * It stops at the first malformed record.
 */
tlv_status_t
tlv_parse_all(const uint8_t *buffer,
              size_t buffer_len,
              size_t *records_parsed)
{
    size_t offset = 0;
    size_t count = 0;
    tlv_view_t view;

    if (records_parsed == NULL)
    {
        return TLV_STATUS_BAD_ARGUMENT;
    }

    *records_parsed = 0;

    if (buffer == NULL && buffer_len != 0)
    {
        return TLV_STATUS_BAD_ARGUMENT;
    }

    while (offset < buffer_len)
    {
        tlv_status_t status =
            tlv_parse_next(buffer,
                           buffer_len,
                           &offset,
                           true, /* complete buffer */
                           &view);

        if (status != TLV_STATUS_OK)
        {
            *records_parsed = count;
            return status;
        }

        ++count;

        printf("TLV #%zu: type=0x%02X, length=%u, value=",
               count,
               (unsigned)view.type,
               (unsigned)view.length);

        for (size_t i = 0; i < (size_t)view.length; ++i)
        {
            printf("%02X ", (unsigned)view.value[i]);
        }

        printf("\n");
    }

    *records_parsed = count;
    return TLV_STATUS_OK;
}

/* A small test helper. */
static void
run_test(const char *name,
         const uint8_t *buffer,
         size_t buffer_len)
{
    size_t count = 0;

    printf("\n--- %s ---\n", name);

    tlv_status_t status =
        tlv_parse_all(buffer, buffer_len, &count);

    printf("Result: %d, records parsed: %zu\n",
           (int)status,
           count);
}

int main(void)
{
    /*
     * Two records:
     *
     * TLV 1:
     *   TYPE   = 0x01
     *   LENGTH = 3
     *   VALUE  = AA BB CC
     *
     * TLV 2:
     *   TYPE   = 0x02
     *   LENGTH = 2
     *   VALUE  = 11 22
     */
    const uint8_t valid_packet[] = {
        0x01, 0x00, 0x03, 0xAA, 0xBB, 0xCC,
        0x02, 0x00, 0x02, 0x11, 0x22};

    /*
     * A valid zero-length value:
     *
     * TYPE   = 0x10
     * LENGTH = 0
     * VALUE  = empty
     */
    const uint8_t zero_length_packet[] = {
        0x10, 0x00, 0x00};

    /*
     * Truncated header: only TYPE and LENGTH high byte exist.
     */
    const uint8_t truncated_header[] = {
        0x01, 0x00};

    /*
     * Header says value length is 4, but only 2 value bytes exist.
     */
    const uint8_t truncated_value[] = {
        0x01, 0x00, 0x04, 0xAA, 0xBB};

    /*
     * Length is 4097, which exceeds TLV_MAX_VALUE_LENGTH.
     * No value bytes are needed to detect this violation.
     */
    const uint8_t too_large_packet[] = {
        0x01, 0x10, 0x01};

    /*
     * First record is valid; second record has a truncated header.
     */
    const uint8_t valid_then_invalid[] = {
        0x01, 0x00, 0x01, 0xAA,
        0x02, 0x00};

    run_test("Two valid TLVs",
             valid_packet,
             sizeof(valid_packet));

    run_test("Zero-length value",
             zero_length_packet,
             sizeof(zero_length_packet));

    run_test("Truncated header",
             truncated_header,
             sizeof(truncated_header));

    run_test("Truncated value",
             truncated_value,
             sizeof(truncated_value));

    run_test("Value too large",
             too_large_packet,
             sizeof(too_large_packet));

    run_test("Valid record followed by invalid record",
             valid_then_invalid,
             sizeof(valid_then_invalid));

    run_test("Empty input",
             NULL,
             0);

    return 0;
}
