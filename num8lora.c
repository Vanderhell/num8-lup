#include "num8lora.h"

#include <stddef.h>

#define NUM8LORA_COMMON_HDR_SIZE 8u
#define NUM8LORA_CRC_SIZE 2u
#define NUM8LORA_BEACON_PAYLOAD_SIZE 18u
#define NUM8LORA_REQ_PAYLOAD_SIZE 8u
#define NUM8LORA_ACK_PAYLOAD_SIZE 8u
#define NUM8LORA_NACK_PAYLOAD_SIZE 8u
#define NUM8LORA_UPDATE_FIXED_PAYLOAD_SIZE 16u
#define NUM8LORA_MAX_VALUE 99999999u

static void st16(uint8_t* p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static void st32(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static uint16_t ld16(const uint8_t* p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t ld32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int valid_number(uint32_t v)
{
    return v <= NUM8LORA_MAX_VALUE;
}

static void encode_common(uint8_t* out, uint8_t msg_type, uint16_t sender_id, uint16_t receiver_id, uint16_t seq)
{
    out[0] = NUM8LORA_PROTOCOL_VERSION;
    out[1] = msg_type;
    st16(&out[2], sender_id);
    st16(&out[4], receiver_id);
    st16(&out[6], seq);
}

static void finalize_crc(uint8_t* out, uint32_t total_len)
{
    uint16_t crc = num8lora_crc16_ccitt_false(out, total_len - 2u);
    st16(out + total_len - 2u, crc);
}

uint16_t num8lora_crc16_ccitt_false(const void* data, uint32_t len)
{
    const uint8_t* p = (const uint8_t*)data;
    uint16_t crc = 0xFFFFu;
    uint32_t i;
    for (i = 0; i < len; ++i)
    {
        int b;
        crc ^= (uint16_t)((uint16_t)p[i] << 8);
        for (b = 0; b < 8; ++b)
        {
            if ((crc & 0x8000u) != 0u)
            {
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            }
            else
            {
                crc = (uint16_t)(crc << 1);
            }
        }
    }
    return crc;
}

int num8lora_encode_beacon(uint8_t* out_buf, uint32_t out_cap, uint16_t sender_id, uint16_t seq, const num8lora_beacon_payload_t* payload, uint32_t* out_len)
{
    uint32_t len = NUM8LORA_COMMON_HDR_SIZE + NUM8LORA_BEACON_PAYLOAD_SIZE + NUM8LORA_CRC_SIZE;
    if (out_buf == NULL || payload == NULL || out_len == NULL || out_cap < len)
    {
        return 0;
    }

    encode_common(out_buf, NUM8LORA_MSG_BEACON, sender_id, 0u, seq);
    st32(&out_buf[8], payload->current_update_id);
    st32(&out_buf[12], payload->dataset_version_from);
    st32(&out_buf[16], payload->dataset_version_to);
    out_buf[20] = payload->remove_count;
    out_buf[21] = payload->add_count;
    st16(&out_buf[22], payload->update_payload_size);
    st16(&out_buf[24], payload->update_payload_crc16);
    finalize_crc(out_buf, len);
    *out_len = len;
    return 1;
}

int num8lora_encode_update_request(uint8_t* out_buf, uint32_t out_cap, uint16_t sender_id, uint16_t receiver_id, uint16_t seq, const num8lora_update_request_payload_t* payload, uint32_t* out_len)
{
    uint32_t len = NUM8LORA_COMMON_HDR_SIZE + NUM8LORA_REQ_PAYLOAD_SIZE + NUM8LORA_CRC_SIZE;
    if (out_buf == NULL || payload == NULL || out_len == NULL || out_cap < len)
    {
        return 0;
    }
    encode_common(out_buf, NUM8LORA_MSG_UPDATE_REQUEST, sender_id, receiver_id, seq);
    st32(&out_buf[8], payload->requested_update_id);
    st32(&out_buf[12], payload->receiver_dataset_ver);
    finalize_crc(out_buf, len);
    *out_len = len;
    return 1;
}

int num8lora_encode_update_data(uint8_t* out_buf, uint32_t out_cap, uint16_t sender_id, uint16_t receiver_id, uint16_t seq, const num8lora_update_header_t* header, const uint32_t* remove_numbers, const uint32_t* add_numbers, uint32_t* out_len)
{
    uint32_t i;
    uint32_t rc;
    uint32_t ac;
    uint32_t off;
    uint32_t len;

    if (out_buf == NULL || header == NULL || out_len == NULL)
    {
        return 0;
    }

    rc = (uint32_t)header->remove_count;
    ac = (uint32_t)header->add_count;
    len = NUM8LORA_COMMON_HDR_SIZE + NUM8LORA_UPDATE_FIXED_PAYLOAD_SIZE + 4u * rc + 4u * ac + NUM8LORA_CRC_SIZE;
    if (out_cap < len || header->reserved0 != 0u)
    {
        return 0;
    }
    if ((rc > 0u && remove_numbers == NULL) || (ac > 0u && add_numbers == NULL))
    {
        return 0;
    }

    encode_common(out_buf, NUM8LORA_MSG_UPDATE_DATA, sender_id, receiver_id, seq);
    st32(&out_buf[8], header->update_id);
    st32(&out_buf[12], header->dataset_version_from);
    st32(&out_buf[16], header->dataset_version_to);
    out_buf[20] = header->remove_count;
    out_buf[21] = header->add_count;
    st16(&out_buf[22], 0u);

    off = 24u;
    for (i = 0; i < rc; ++i)
    {
        st32(&out_buf[off], remove_numbers[i]);
        off += 4u;
    }
    for (i = 0; i < ac; ++i)
    {
        st32(&out_buf[off], add_numbers[i]);
        off += 4u;
    }

    finalize_crc(out_buf, len);
    *out_len = len;
    return 1;
}

int num8lora_encode_ack(uint8_t* out_buf, uint32_t out_cap, uint16_t sender_id, uint16_t receiver_id, uint16_t seq, const num8lora_ack_payload_t* payload, uint32_t* out_len)
{
    uint32_t len = NUM8LORA_COMMON_HDR_SIZE + NUM8LORA_ACK_PAYLOAD_SIZE + NUM8LORA_CRC_SIZE;
    if (out_buf == NULL || payload == NULL || out_len == NULL || out_cap < len)
    {
        return 0;
    }
    encode_common(out_buf, NUM8LORA_MSG_ACK, sender_id, receiver_id, seq);
    st32(&out_buf[8], payload->ack_update_id);
    st32(&out_buf[12], payload->receiver_dataset_ver);
    finalize_crc(out_buf, len);
    *out_len = len;
    return 1;
}

int num8lora_encode_nack(uint8_t* out_buf, uint32_t out_cap, uint16_t sender_id, uint16_t receiver_id, uint16_t seq, const num8lora_nack_payload_t* payload, uint32_t* out_len)
{
    uint32_t len = NUM8LORA_COMMON_HDR_SIZE + NUM8LORA_NACK_PAYLOAD_SIZE + NUM8LORA_CRC_SIZE;
    if (out_buf == NULL || payload == NULL || out_len == NULL || out_cap < len)
    {
        return 0;
    }
    encode_common(out_buf, NUM8LORA_MSG_NACK, sender_id, receiver_id, seq);
    st32(&out_buf[8], payload->nack_update_id);
    st16(&out_buf[12], payload->error_code);
    st16(&out_buf[14], payload->detail);
    finalize_crc(out_buf, len);
    *out_len = len;
    return 1;
}

int num8lora_decode_common_header(const uint8_t* buf, uint32_t len, num8lora_common_header_t* out_hdr)
{
    if (buf == NULL || out_hdr == NULL || len < NUM8LORA_COMMON_HDR_SIZE)
    {
        return 0;
    }
    out_hdr->protocol_version = buf[0];
    out_hdr->msg_type = buf[1];
    out_hdr->sender_id = ld16(&buf[2]);
    out_hdr->receiver_id = ld16(&buf[4]);
    out_hdr->seq = ld16(&buf[6]);
    return 1;
}

int num8lora_validate_frame_crc(const uint8_t* buf, uint32_t len)
{
    uint16_t given;
    uint16_t calc;
    if (buf == NULL || len < NUM8LORA_COMMON_HDR_SIZE + NUM8LORA_CRC_SIZE)
    {
        return 0;
    }
    given = ld16(buf + len - 2u);
    calc = num8lora_crc16_ccitt_false(buf, len - 2u);
    return given == calc;
}

static int decode_header_and_crc(const uint8_t* buf, uint32_t len, uint8_t type, num8lora_common_header_t* out_hdr)
{
    if (!num8lora_validate_frame_crc(buf, len))
    {
        return 0;
    }
    if (!num8lora_decode_common_header(buf, len, out_hdr))
    {
        return 0;
    }
    if (out_hdr->protocol_version != NUM8LORA_PROTOCOL_VERSION || out_hdr->msg_type != type)
    {
        return 0;
    }
    return 1;
}

int num8lora_decode_beacon(const uint8_t* buf, uint32_t len, num8lora_common_header_t* out_hdr, num8lora_beacon_payload_t* out_payload)
{
    if (buf == NULL || out_hdr == NULL || out_payload == NULL || len != 28u)
    {
        return 0;
    }
    if (!decode_header_and_crc(buf, len, NUM8LORA_MSG_BEACON, out_hdr))
    {
        return 0;
    }
    out_payload->current_update_id = ld32(&buf[8]);
    out_payload->dataset_version_from = ld32(&buf[12]);
    out_payload->dataset_version_to = ld32(&buf[16]);
    out_payload->remove_count = buf[20];
    out_payload->add_count = buf[21];
    out_payload->update_payload_size = ld16(&buf[22]);
    out_payload->update_payload_crc16 = ld16(&buf[24]);
    if (out_hdr->receiver_id != 0u)
    {
        return 0;
    }
    return 1;
}

int num8lora_decode_update_request(const uint8_t* buf, uint32_t len, num8lora_common_header_t* out_hdr, num8lora_update_request_payload_t* out_payload)
{
    if (buf == NULL || out_hdr == NULL || out_payload == NULL || len != 18u)
    {
        return 0;
    }
    if (!decode_header_and_crc(buf, len, NUM8LORA_MSG_UPDATE_REQUEST, out_hdr))
    {
        return 0;
    }
    out_payload->requested_update_id = ld32(&buf[8]);
    out_payload->receiver_dataset_ver = ld32(&buf[12]);
    return 1;
}

int num8lora_decode_update_data_header(const uint8_t* buf, uint32_t len, num8lora_common_header_t* out_hdr, num8lora_update_header_t* out_upd_hdr, const uint8_t** out_remove_ptr, const uint8_t** out_add_ptr)
{
    uint32_t rc;
    uint32_t ac;
    uint32_t expected;
    if (buf == NULL || out_hdr == NULL || out_upd_hdr == NULL || out_remove_ptr == NULL || out_add_ptr == NULL)
    {
        return 0;
    }
    if (!decode_header_and_crc(buf, len, NUM8LORA_MSG_UPDATE_DATA, out_hdr))
    {
        return 0;
    }
    if (len < 26u)
    {
        return 0;
    }

    out_upd_hdr->update_id = ld32(&buf[8]);
    out_upd_hdr->dataset_version_from = ld32(&buf[12]);
    out_upd_hdr->dataset_version_to = ld32(&buf[16]);
    out_upd_hdr->remove_count = buf[20];
    out_upd_hdr->add_count = buf[21];
    out_upd_hdr->reserved0 = ld16(&buf[22]);

    rc = (uint32_t)out_upd_hdr->remove_count;
    ac = (uint32_t)out_upd_hdr->add_count;
    expected = 26u + 4u * rc + 4u * ac;
    if (len != expected || out_upd_hdr->reserved0 != 0u)
    {
        return 0;
    }

    *out_remove_ptr = &buf[24];
    *out_add_ptr = &buf[24 + 4u * rc];
    return 1;
}

int num8lora_decode_ack(const uint8_t* buf, uint32_t len, num8lora_common_header_t* out_hdr, num8lora_ack_payload_t* out_payload)
{
    if (buf == NULL || out_hdr == NULL || out_payload == NULL || len != 18u)
    {
        return 0;
    }
    if (!decode_header_and_crc(buf, len, NUM8LORA_MSG_ACK, out_hdr))
    {
        return 0;
    }
    out_payload->ack_update_id = ld32(&buf[8]);
    out_payload->receiver_dataset_ver = ld32(&buf[12]);
    return 1;
}

int num8lora_decode_nack(const uint8_t* buf, uint32_t len, num8lora_common_header_t* out_hdr, num8lora_nack_payload_t* out_payload)
{
    if (buf == NULL || out_hdr == NULL || out_payload == NULL || len != 18u)
    {
        return 0;
    }
    if (!decode_header_and_crc(buf, len, NUM8LORA_MSG_NACK, out_hdr))
    {
        return 0;
    }
    out_payload->nack_update_id = ld32(&buf[8]);
    out_payload->error_code = ld16(&buf[12]);
    out_payload->detail = ld16(&buf[14]);
    return 1;
}

int num8lora_validate_update_numbers(const num8lora_update_header_t* hdr, const uint32_t* remove_numbers, const uint32_t* add_numbers, uint16_t* out_error_code)
{
    uint32_t rc;
    uint32_t ac;
    uint32_t i;
    uint32_t j;

    if (out_error_code != NULL)
    {
        *out_error_code = NUM8LORA_ERR_NONE;
    }
    if (hdr == NULL)
    {
        return 0;
    }

    rc = (uint32_t)hdr->remove_count;
    ac = (uint32_t)hdr->add_count;

    if ((rc > 0u && remove_numbers == NULL) || (ac > 0u && add_numbers == NULL))
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_ERR_INTERNAL;
        }
        return 0;
    }

    for (i = 0; i < rc; ++i)
    {
        if (!valid_number(remove_numbers[i]))
        {
            if (out_error_code != NULL)
            {
                *out_error_code = NUM8LORA_ERR_INVALID_NUMBER;
            }
            return 0;
        }
        for (j = i + 1u; j < rc; ++j)
        {
            if (remove_numbers[i] == remove_numbers[j])
            {
                if (out_error_code != NULL)
                {
                    *out_error_code = NUM8LORA_ERR_DUPLICATE_IN_BATCH;
                }
                return 0;
            }
        }
    }

    for (i = 0; i < ac; ++i)
    {
        if (!valid_number(add_numbers[i]))
        {
            if (out_error_code != NULL)
            {
                *out_error_code = NUM8LORA_ERR_INVALID_NUMBER;
            }
            return 0;
        }
        for (j = i + 1u; j < ac; ++j)
        {
            if (add_numbers[i] == add_numbers[j])
            {
                if (out_error_code != NULL)
                {
                    *out_error_code = NUM8LORA_ERR_DUPLICATE_IN_BATCH;
                }
                return 0;
            }
        }
    }

    for (i = 0; i < rc; ++i)
    {
        for (j = 0; j < ac; ++j)
        {
            if (remove_numbers[i] == add_numbers[j])
            {
                if (out_error_code != NULL)
                {
                    *out_error_code = NUM8LORA_ERR_NUMBER_IN_BOTH;
                }
                return 0;
            }
        }
    }

    return 1;
}

int num8lora_decode_update_numbers(
    const num8lora_update_header_t* hdr,
    const uint8_t* remove_ptr,
    const uint8_t* add_ptr,
    uint32_t* out_remove_numbers,
    uint32_t* out_add_numbers)
{
    uint32_t i;
    uint32_t rc;
    uint32_t ac;

    if (hdr == NULL || remove_ptr == NULL || add_ptr == NULL)
    {
        return 0;
    }

    rc = (uint32_t)hdr->remove_count;
    ac = (uint32_t)hdr->add_count;

    if ((rc > 0u && out_remove_numbers == NULL) || (ac > 0u && out_add_numbers == NULL))
    {
        return 0;
    }

    for (i = 0; i < rc; ++i)
    {
        out_remove_numbers[i] = ld32(remove_ptr + 4u * i);
    }
    for (i = 0; i < ac; ++i)
    {
        out_add_numbers[i] = ld32(add_ptr + 4u * i);
    }

    return 1;
}

int num8lora_compute_update_payload_crc16(
    const num8lora_update_header_t* hdr,
    const uint32_t* remove_numbers,
    const uint32_t* add_numbers,
    uint16_t* out_crc16)
{
    uint8_t temp[16 + 4u * 255u + 4u * 255u];
    uint32_t off = 0;
    uint32_t i;

    if (hdr == NULL || out_crc16 == NULL)
    {
        return 0;
    }
    if ((hdr->remove_count > 0u && remove_numbers == NULL) || (hdr->add_count > 0u && add_numbers == NULL))
    {
        return 0;
    }

    st32(&temp[off], hdr->update_id); off += 4u;
    st32(&temp[off], hdr->dataset_version_from); off += 4u;
    st32(&temp[off], hdr->dataset_version_to); off += 4u;
    temp[off++] = hdr->remove_count;
    temp[off++] = hdr->add_count;
    st16(&temp[off], 0u); off += 2u;

    for (i = 0; i < (uint32_t)hdr->remove_count; ++i)
    {
        st32(&temp[off], remove_numbers[i]);
        off += 4u;
    }
    for (i = 0; i < (uint32_t)hdr->add_count; ++i)
    {
        st32(&temp[off], add_numbers[i]);
        off += 4u;
    }

    *out_crc16 = num8lora_crc16_ccitt_false(temp, off);
    return 1;
}

int num8lora_classify_frame(
    const uint8_t* buf,
    uint32_t len,
    uint16_t expected_receiver_id,
    uint8_t expected_msg_type,
    num8lora_common_header_t* out_hdr,
    uint16_t* out_error_code)
{
    num8lora_common_header_t hdr;

    if (out_error_code != NULL)
    {
        *out_error_code = NUM8LORA_ERR_NONE;
    }
    if (buf == NULL || out_hdr == NULL)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_ERR_INTERNAL;
        }
        return 0;
    }

    if (!num8lora_validate_frame_crc(buf, len))
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_ERR_CRC_FAIL;
        }
        return 0;
    }

    if (!num8lora_decode_common_header(buf, len, &hdr))
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_ERR_INTERNAL;
        }
        return 0;
    }

    if (hdr.protocol_version != NUM8LORA_PROTOCOL_VERSION)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_ERR_PROTOCOL_VERSION_UNSUPPORTED;
        }
        return 0;
    }

    if (hdr.msg_type < NUM8LORA_MSG_BEACON || hdr.msg_type > NUM8LORA_MSG_NACK)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_ERR_INVALID_MESSAGE_TYPE;
        }
        return 0;
    }

    if (expected_msg_type != 0u && hdr.msg_type != expected_msg_type)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_ERR_INVALID_MESSAGE_TYPE;
        }
        return 0;
    }

    if (expected_receiver_id != NUM8LORA_ANY_RECEIVER_ID)
    {
        if (hdr.msg_type == NUM8LORA_MSG_BEACON)
        {
            if (hdr.receiver_id != 0u)
            {
                if (out_error_code != NULL)
                {
                    *out_error_code = NUM8LORA_ERR_INVALID_RECEIVER_ID;
                }
                return 0;
            }
        }
        else if (hdr.receiver_id != expected_receiver_id)
        {
            if (out_error_code != NULL)
            {
                *out_error_code = NUM8LORA_ERR_INVALID_RECEIVER_ID;
            }
            return 0;
        }
    }

    *out_hdr = hdr;
    return 1;
}
