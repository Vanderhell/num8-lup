#include "num8lora_op.h"
#include "num8lora_codec.h"

#define NUM8LORA_OP_FRAME_HDR_SIZE 8u
#define NUM8LORA_OP_FRAME_CRC_SIZE 2u
#define NUM8LORA_OP_BEACON_PAYLOAD_SIZE 8u
#define NUM8LORA_OP_REQUEST_PAYLOAD_SIZE 8u
#define NUM8LORA_OP_DATA_PAYLOAD_SIZE 16u
#define NUM8LORA_OP_ACK_PAYLOAD_SIZE 8u
#define NUM8LORA_OP_NACK_PAYLOAD_SIZE 12u
#define NUM8LORA_OP_MAX_VALUE 99999999u

static int num8lora_op_payload_len_add_u32(uint32_t a, uint32_t b, uint32_t* out)
{
    return num8lora_codec_add_u32(a, b, out);
}

static int num8lora_op_frame_len(uint32_t payload_len, uint32_t* out_len)
{
    uint32_t len = 0u;

    if (!num8lora_op_payload_len_add_u32(NUM8LORA_OP_FRAME_HDR_SIZE, payload_len, &len))
    {
        return 0;
    }
    return num8lora_op_payload_len_add_u32(len, NUM8LORA_OP_FRAME_CRC_SIZE, out_len);
}

static void num8lora_op_clear_common(num8lora_op_common_header_t* out_hdr)
{
    if (out_hdr != NULL)
    {
        out_hdr->protocol_version = 0u;
        out_hdr->msg_type = 0u;
        out_hdr->sender_id = 0u;
        out_hdr->receiver_id = 0u;
        out_hdr->seq = 0u;
    }
}

static void num8lora_op_clear_beacon(num8lora_op_beacon_payload_t* out_p)
{
    if (out_p != NULL)
    {
        out_p->stream_id = 0u;
        out_p->latest_op_id = 0u;
    }
}

static void num8lora_op_clear_request(num8lora_op_request_payload_t* out_p)
{
    if (out_p != NULL)
    {
        out_p->stream_id = 0u;
        out_p->next_needed_op_id = 0u;
    }
}

static void num8lora_op_clear_data(num8lora_op_data_payload_t* out_p)
{
    if (out_p != NULL)
    {
        out_p->stream_id = 0u;
        out_p->op_id = 0u;
        out_p->op_type = 0u;
        out_p->reserved0 = 0u;
        out_p->reserved1 = 0u;
        out_p->value = 0u;
    }
}

static void num8lora_op_clear_ack(num8lora_op_ack_payload_t* out_p)
{
    if (out_p != NULL)
    {
        out_p->stream_id = 0u;
        out_p->ack_op_id = 0u;
    }
}

static void num8lora_op_clear_nack(num8lora_op_nack_payload_t* out_p)
{
    if (out_p != NULL)
    {
        out_p->stream_id = 0u;
        out_p->error_code = 0u;
        out_p->detail = 0u;
        out_p->expected_next_op_id = 0u;
    }
}

static int num8lora_op_validate_common_header(const uint8_t* buf, uint32_t len, uint8_t expected_type, num8lora_op_common_header_t* out_hdr)
{
    if (!num8lora_op_validate_frame_crc(buf, len) || !num8lora_op_decode_common_header(buf, len, out_hdr))
    {
        return 0;
    }
    if (out_hdr->protocol_version != NUM8LORA_OP_PROTOCOL_VERSION || out_hdr->msg_type != expected_type)
    {
        return 0;
    }
    if (out_hdr->sender_id == 0u)
    {
        return 0;
    }
    if (expected_type == NUM8LORA_OP_MSG_BEACON)
    {
        return out_hdr->receiver_id == 0u;
    }
    return out_hdr->receiver_id != 0u;
}

uint16_t num8lora_op_crc16_ccitt_false(const void* data, uint32_t len)
{
    return num8lora_codec_crc16_ccitt_false(data, len);
}

int num8lora_op_encode_beacon(uint8_t* out_buf, uint32_t out_cap, uint16_t sender_id, uint16_t seq, const num8lora_op_beacon_payload_t* p, uint32_t* out_len)
{
    uint32_t len = 0u;

    if (out_len != NULL)
    {
        *out_len = 0u;
    }
    if (out_buf == NULL || p == NULL || out_len == NULL)
    {
        return 0;
    }
    if (p->stream_id == 0u || p->latest_op_id == 0u)
    {
        return 0;
    }
    if (!num8lora_op_frame_len(NUM8LORA_OP_BEACON_PAYLOAD_SIZE, &len) || out_cap < len || sender_id == 0u)
    {
        return 0;
    }

    out_buf[0] = NUM8LORA_OP_PROTOCOL_VERSION;
    out_buf[1] = NUM8LORA_OP_MSG_BEACON;
    num8lora_codec_write_u16_le(&out_buf[2], sender_id);
    num8lora_codec_write_u16_le(&out_buf[4], 0u);
    num8lora_codec_write_u16_le(&out_buf[6], seq);
    num8lora_codec_write_u32_le(&out_buf[8], p->stream_id);
    num8lora_codec_write_u32_le(&out_buf[12], p->latest_op_id);
    num8lora_codec_write_u16_le(&out_buf[len - 2u], num8lora_op_crc16_ccitt_false(out_buf, len - 2u));
    *out_len = len;
    return 1;
}

int num8lora_op_encode_request(uint8_t* out_buf, uint32_t out_cap, uint16_t sender_id, uint16_t receiver_id, uint16_t seq, const num8lora_op_request_payload_t* p, uint32_t* out_len)
{
    uint32_t len = 0u;

    if (out_len != NULL)
    {
        *out_len = 0u;
    }
    if (out_buf == NULL || p == NULL || out_len == NULL)
    {
        return 0;
    }
    if (sender_id == 0u || receiver_id == 0u || p->stream_id == 0u || p->next_needed_op_id == 0u)
    {
        return 0;
    }
    if (!num8lora_op_frame_len(NUM8LORA_OP_REQUEST_PAYLOAD_SIZE, &len) || out_cap < len)
    {
        return 0;
    }

    out_buf[0] = NUM8LORA_OP_PROTOCOL_VERSION;
    out_buf[1] = NUM8LORA_OP_MSG_REQUEST;
    num8lora_codec_write_u16_le(&out_buf[2], sender_id);
    num8lora_codec_write_u16_le(&out_buf[4], receiver_id);
    num8lora_codec_write_u16_le(&out_buf[6], seq);
    num8lora_codec_write_u32_le(&out_buf[8], p->stream_id);
    num8lora_codec_write_u32_le(&out_buf[12], p->next_needed_op_id);
    num8lora_codec_write_u16_le(&out_buf[len - 2u], num8lora_op_crc16_ccitt_false(out_buf, len - 2u));
    *out_len = len;
    return 1;
}

int num8lora_op_encode_data(uint8_t* out_buf, uint32_t out_cap, uint16_t sender_id, uint16_t receiver_id, uint16_t seq, const num8lora_op_data_payload_t* p, uint32_t* out_len)
{
    uint32_t len = 0u;

    if (out_len != NULL)
    {
        *out_len = 0u;
    }
    if (out_buf == NULL || p == NULL || out_len == NULL)
    {
        return 0;
    }
    if (sender_id == 0u || receiver_id == 0u || p->stream_id == 0u || p->op_id == 0u || p->reserved0 != 0u || p->reserved1 != 0u)
    {
        return 0;
    }
    if (p->op_type != NUM8LORA_OP_ADD && p->op_type != NUM8LORA_OP_REMOVE)
    {
        return 0;
    }
    if (p->value > NUM8LORA_OP_MAX_VALUE)
    {
        return 0;
    }
    if (!num8lora_op_frame_len(NUM8LORA_OP_DATA_PAYLOAD_SIZE, &len) || out_cap < len)
    {
        return 0;
    }

    out_buf[0] = NUM8LORA_OP_PROTOCOL_VERSION;
    out_buf[1] = NUM8LORA_OP_MSG_DATA;
    num8lora_codec_write_u16_le(&out_buf[2], sender_id);
    num8lora_codec_write_u16_le(&out_buf[4], receiver_id);
    num8lora_codec_write_u16_le(&out_buf[6], seq);
    num8lora_codec_write_u32_le(&out_buf[8], p->stream_id);
    num8lora_codec_write_u32_le(&out_buf[12], p->op_id);
    out_buf[16] = p->op_type;
    out_buf[17] = 0u;
    num8lora_codec_write_u16_le(&out_buf[18], 0u);
    num8lora_codec_write_u32_le(&out_buf[20], p->value);
    num8lora_codec_write_u16_le(&out_buf[len - 2u], num8lora_op_crc16_ccitt_false(out_buf, len - 2u));
    *out_len = len;
    return 1;
}

int num8lora_op_encode_ack(uint8_t* out_buf, uint32_t out_cap, uint16_t sender_id, uint16_t receiver_id, uint16_t seq, const num8lora_op_ack_payload_t* p, uint32_t* out_len)
{
    uint32_t len = 0u;

    if (out_len != NULL)
    {
        *out_len = 0u;
    }
    if (out_buf == NULL || p == NULL || out_len == NULL)
    {
        return 0;
    }
    if (sender_id == 0u || receiver_id == 0u || p->stream_id == 0u || p->ack_op_id == 0u)
    {
        return 0;
    }
    if (!num8lora_op_frame_len(NUM8LORA_OP_ACK_PAYLOAD_SIZE, &len) || out_cap < len)
    {
        return 0;
    }

    out_buf[0] = NUM8LORA_OP_PROTOCOL_VERSION;
    out_buf[1] = NUM8LORA_OP_MSG_ACK;
    num8lora_codec_write_u16_le(&out_buf[2], sender_id);
    num8lora_codec_write_u16_le(&out_buf[4], receiver_id);
    num8lora_codec_write_u16_le(&out_buf[6], seq);
    num8lora_codec_write_u32_le(&out_buf[8], p->stream_id);
    num8lora_codec_write_u32_le(&out_buf[12], p->ack_op_id);
    num8lora_codec_write_u16_le(&out_buf[len - 2u], num8lora_op_crc16_ccitt_false(out_buf, len - 2u));
    *out_len = len;
    return 1;
}

int num8lora_op_encode_nack(uint8_t* out_buf, uint32_t out_cap, uint16_t sender_id, uint16_t receiver_id, uint16_t seq, const num8lora_op_nack_payload_t* p, uint32_t* out_len)
{
    uint32_t len = 0u;

    if (out_len != NULL)
    {
        *out_len = 0u;
    }
    if (out_buf == NULL || p == NULL || out_len == NULL)
    {
        return 0;
    }
    if (sender_id == 0u || receiver_id == 0u || p->stream_id == 0u || p->expected_next_op_id == 0u)
    {
        return 0;
    }
    if (!num8lora_op_frame_len(NUM8LORA_OP_NACK_PAYLOAD_SIZE, &len) || out_cap < len)
    {
        return 0;
    }

    out_buf[0] = NUM8LORA_OP_PROTOCOL_VERSION;
    out_buf[1] = NUM8LORA_OP_MSG_NACK;
    num8lora_codec_write_u16_le(&out_buf[2], sender_id);
    num8lora_codec_write_u16_le(&out_buf[4], receiver_id);
    num8lora_codec_write_u16_le(&out_buf[6], seq);
    num8lora_codec_write_u32_le(&out_buf[8], p->stream_id);
    num8lora_codec_write_u16_le(&out_buf[12], p->error_code);
    num8lora_codec_write_u16_le(&out_buf[14], p->detail);
    num8lora_codec_write_u32_le(&out_buf[16], p->expected_next_op_id);
    num8lora_codec_write_u16_le(&out_buf[len - 2u], num8lora_op_crc16_ccitt_false(out_buf, len - 2u));
    *out_len = len;
    return 1;
}

int num8lora_op_decode_common_header(const uint8_t* b, uint32_t len, num8lora_op_common_header_t* o)
{
    if (o != NULL)
    {
        num8lora_op_clear_common(o);
    }
    if (b == NULL || o == NULL || len < NUM8LORA_OP_FRAME_HDR_SIZE)
    {
        return 0;
    }
    o->protocol_version = b[0];
    o->msg_type = b[1];
    o->sender_id = num8lora_codec_read_u16_le(&b[2]);
    o->receiver_id = num8lora_codec_read_u16_le(&b[4]);
    o->seq = num8lora_codec_read_u16_le(&b[6]);
    return 1;
}

int num8lora_op_validate_frame_crc(const uint8_t* b, uint32_t len)
{
    if (b == NULL || len < NUM8LORA_OP_FRAME_HDR_SIZE + NUM8LORA_OP_FRAME_CRC_SIZE)
    {
        return 0;
    }
    return num8lora_codec_read_u16_le(&b[len - 2u]) == num8lora_op_crc16_ccitt_false(b, len - 2u);
}

int num8lora_op_decode_beacon(const uint8_t* b, uint32_t len, num8lora_op_common_header_t* h, num8lora_op_beacon_payload_t* p)
{
    if (p != NULL)
    {
        num8lora_op_clear_beacon(p);
    }
    if (b == NULL || h == NULL || p == NULL || len != (NUM8LORA_OP_FRAME_HDR_SIZE + NUM8LORA_OP_BEACON_PAYLOAD_SIZE + NUM8LORA_OP_FRAME_CRC_SIZE))
    {
        return 0;
    }
    if (!num8lora_op_validate_common_header(b, len, NUM8LORA_OP_MSG_BEACON, h))
    {
        return 0;
    }

    p->stream_id = num8lora_codec_read_u32_le(&b[8]);
    p->latest_op_id = num8lora_codec_read_u32_le(&b[12]);
    return p->stream_id != 0u && p->latest_op_id != 0u;
}

int num8lora_op_decode_request(const uint8_t* b, uint32_t len, num8lora_op_common_header_t* h, num8lora_op_request_payload_t* p)
{
    if (p != NULL)
    {
        num8lora_op_clear_request(p);
    }
    if (b == NULL || h == NULL || p == NULL || len != (NUM8LORA_OP_FRAME_HDR_SIZE + NUM8LORA_OP_REQUEST_PAYLOAD_SIZE + NUM8LORA_OP_FRAME_CRC_SIZE))
    {
        return 0;
    }
    if (!num8lora_op_validate_common_header(b, len, NUM8LORA_OP_MSG_REQUEST, h))
    {
        return 0;
    }

    p->stream_id = num8lora_codec_read_u32_le(&b[8]);
    p->next_needed_op_id = num8lora_codec_read_u32_le(&b[12]);
    return p->stream_id != 0u && p->next_needed_op_id != 0u;
}

int num8lora_op_decode_data(const uint8_t* b, uint32_t len, num8lora_op_common_header_t* h, num8lora_op_data_payload_t* p)
{
    if (p != NULL)
    {
        num8lora_op_clear_data(p);
    }
    if (b == NULL || h == NULL || p == NULL || len != (NUM8LORA_OP_FRAME_HDR_SIZE + NUM8LORA_OP_DATA_PAYLOAD_SIZE + NUM8LORA_OP_FRAME_CRC_SIZE))
    {
        return 0;
    }
    if (!num8lora_op_validate_common_header(b, len, NUM8LORA_OP_MSG_DATA, h))
    {
        return 0;
    }

    p->stream_id = num8lora_codec_read_u32_le(&b[8]);
    p->op_id = num8lora_codec_read_u32_le(&b[12]);
    p->op_type = b[16];
    p->reserved0 = b[17];
    p->reserved1 = num8lora_codec_read_u16_le(&b[18]);
    p->value = num8lora_codec_read_u32_le(&b[20]);
    if (p->stream_id == 0u || p->op_id == 0u || p->reserved0 != 0u || p->reserved1 != 0u)
    {
        num8lora_op_clear_data(p);
        return 0;
    }
    if (p->op_type != NUM8LORA_OP_ADD && p->op_type != NUM8LORA_OP_REMOVE)
    {
        num8lora_op_clear_data(p);
        return 0;
    }
    if (p->value > NUM8LORA_OP_MAX_VALUE)
    {
        num8lora_op_clear_data(p);
        return 0;
    }
    return 1;
}

int num8lora_op_decode_ack(const uint8_t* b, uint32_t len, num8lora_op_common_header_t* h, num8lora_op_ack_payload_t* p)
{
    if (p != NULL)
    {
        num8lora_op_clear_ack(p);
    }
    if (b == NULL || h == NULL || p == NULL || len != (NUM8LORA_OP_FRAME_HDR_SIZE + NUM8LORA_OP_ACK_PAYLOAD_SIZE + NUM8LORA_OP_FRAME_CRC_SIZE))
    {
        return 0;
    }
    if (!num8lora_op_validate_common_header(b, len, NUM8LORA_OP_MSG_ACK, h))
    {
        return 0;
    }

    p->stream_id = num8lora_codec_read_u32_le(&b[8]);
    p->ack_op_id = num8lora_codec_read_u32_le(&b[12]);
    return p->stream_id != 0u && p->ack_op_id != 0u;
}

int num8lora_op_decode_nack(const uint8_t* b, uint32_t len, num8lora_op_common_header_t* h, num8lora_op_nack_payload_t* p)
{
    if (p != NULL)
    {
        num8lora_op_clear_nack(p);
    }
    if (b == NULL || h == NULL || p == NULL || len != (NUM8LORA_OP_FRAME_HDR_SIZE + NUM8LORA_OP_NACK_PAYLOAD_SIZE + NUM8LORA_OP_FRAME_CRC_SIZE))
    {
        return 0;
    }
    if (!num8lora_op_validate_common_header(b, len, NUM8LORA_OP_MSG_NACK, h))
    {
        return 0;
    }

    p->stream_id = num8lora_codec_read_u32_le(&b[8]);
    p->error_code = num8lora_codec_read_u16_le(&b[12]);
    p->detail = num8lora_codec_read_u16_le(&b[14]);
    p->expected_next_op_id = num8lora_codec_read_u32_le(&b[16]);
    return p->stream_id != 0u && p->expected_next_op_id != 0u;
}
