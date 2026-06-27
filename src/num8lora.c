#include "num8lora.h"
#include "num8lora_codec.h"

#include <stddef.h>

#define NUM8LORA_COMMON_HDR_SIZE 8u
#define NUM8LORA_CRC_SIZE 2u
#define NUM8LORA_BEACON_PAYLOAD_SIZE 18u
#define NUM8LORA_REQ_PAYLOAD_SIZE 8u
#define NUM8LORA_ACK_PAYLOAD_SIZE 8u
#define NUM8LORA_NACK_PAYLOAD_SIZE 8u
#define NUM8LORA_UPDATE_FIXED_PAYLOAD_SIZE 16u
#define NUM8LORA_MAX_VALUE 99999999u

static int add_len_u32(uint32_t a, uint32_t b, uint32_t* out)
{
    return num8lora_codec_add_u32(a, b, out);
}

static int frame_len_u32(uint32_t payload, uint32_t* out)
{
    uint32_t len = 0u;

    if (!add_len_u32(NUM8LORA_COMMON_HDR_SIZE, payload, &len))
    {
        return 0;
    }
    return add_len_u32(len, NUM8LORA_CRC_SIZE, out);
}

static void st16(uint8_t* p, uint16_t v) { num8lora_codec_write_u16_le(p, v); }
static void st32(uint8_t* p, uint32_t v) { num8lora_codec_write_u32_le(p, v); }
static uint16_t ld16(const uint8_t* p) { return num8lora_codec_read_u16_le(p); }
static uint32_t ld32(const uint8_t* p) { return num8lora_codec_read_u32_le(p); }
static uint16_t crc16_update_byte(uint16_t crc, uint8_t byte)
{
    uint32_t bit;

    crc ^= (uint16_t)((uint16_t)byte << 8);
    for (bit = 0u; bit < 8u; ++bit)
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
    return crc;
}

static uint16_t crc16_update_u16(uint16_t crc, uint16_t v)
{
    crc = crc16_update_byte(crc, (uint8_t)(v & 0xFFu));
    return crc16_update_byte(crc, (uint8_t)((v >> 8) & 0xFFu));
}

static uint16_t crc16_update_u32(uint16_t crc, uint32_t v)
{
    crc = crc16_update_byte(crc, (uint8_t)(v & 0xFFu));
    crc = crc16_update_byte(crc, (uint8_t)((v >> 8) & 0xFFu));
    crc = crc16_update_byte(crc, (uint8_t)((v >> 16) & 0xFFu));
    return crc16_update_byte(crc, (uint8_t)((v >> 24) & 0xFFu));
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
    uint16_t crc = num8lora_codec_crc16_ccitt_false(out, total_len - 2u);
    st16(out + total_len - 2u, crc);
}

uint16_t num8lora_crc16_ccitt_false(const void* data, uint32_t len)
{
    return num8lora_codec_crc16_ccitt_false(data, len);
}

int num8lora_encode_beacon(uint8_t* out_buf, uint32_t out_cap, uint16_t sender_id, uint16_t seq, const num8lora_beacon_payload_t* payload, uint32_t* out_len)
{
    uint32_t len = 0u;
    uint32_t expected_size = 0u;

    if (out_len != NULL)
    {
        *out_len = 0u;
    }
    if (out_buf == NULL || payload == NULL || out_len == NULL || sender_id == 0u)
    {
        return 0;
    }
    if (payload->current_update_id == 0u || payload->dataset_version_to <= payload->dataset_version_from)
    {
        return 0;
    }
    if (!add_len_u32(16u, 4u * (uint32_t)payload->remove_count, &expected_size))
    {
        return 0;
    }
    if (!add_len_u32(expected_size, 4u * (uint32_t)payload->add_count, &expected_size))
    {
        return 0;
    }
    if (payload->update_payload_size != expected_size)
    {
        return 0;
    }
    if (!frame_len_u32(NUM8LORA_BEACON_PAYLOAD_SIZE, &len) || out_cap < len)
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
    uint32_t len = 0u;

    if (out_len != NULL)
    {
        *out_len = 0u;
    }
    if (out_buf == NULL || payload == NULL || out_len == NULL || sender_id == 0u || receiver_id == 0u)
    {
        return 0;
    }
    if (payload->requested_update_id == 0u)
    {
        return 0;
    }
    if (!frame_len_u32(NUM8LORA_REQ_PAYLOAD_SIZE, &len) || out_cap < len)
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
    uint32_t payload_len;

    if (out_len != NULL)
    {
        *out_len = 0u;
    }
    if (out_buf == NULL || header == NULL || out_len == NULL || sender_id == 0u || receiver_id == 0u)
    {
        return 0;
    }

    rc = (uint32_t)header->remove_count;
    ac = (uint32_t)header->add_count;
    if (header->update_id == 0u || header->dataset_version_to <= header->dataset_version_from || header->reserved0 != 0u)
    {
        return 0;
    }
    if (!num8lora_codec_mul_u32(rc, 4u, &payload_len))
    {
        return 0;
    }
    if (!num8lora_codec_add_u32(NUM8LORA_UPDATE_FIXED_PAYLOAD_SIZE, payload_len, &payload_len))
    {
        return 0;
    }
    if (!num8lora_codec_mul_u32(ac, 4u, &len))
    {
        return 0;
    }
    if (!num8lora_codec_add_u32(payload_len, len, &payload_len))
    {
        return 0;
    }
    if (!frame_len_u32(payload_len, &len) || out_cap < len)
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
    uint32_t len = 0u;

    if (out_len != NULL)
    {
        *out_len = 0u;
    }
    if (out_buf == NULL || payload == NULL || out_len == NULL || sender_id == 0u || receiver_id == 0u)
    {
        return 0;
    }
    if (payload->ack_update_id == 0u || payload->receiver_dataset_ver == 0u)
    {
        return 0;
    }
    if (!frame_len_u32(NUM8LORA_ACK_PAYLOAD_SIZE, &len) || out_cap < len)
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
    uint32_t len = 0u;

    if (out_len != NULL)
    {
        *out_len = 0u;
    }
    if (out_buf == NULL || payload == NULL || out_len == NULL || sender_id == 0u || receiver_id == 0u)
    {
        return 0;
    }
    if (payload->nack_update_id == 0u)
    {
        return 0;
    }
    if (!frame_len_u32(NUM8LORA_NACK_PAYLOAD_SIZE, &len) || out_cap < len)
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
    if (out_hdr != NULL)
    {
        out_hdr->protocol_version = 0u;
        out_hdr->msg_type = 0u;
        out_hdr->sender_id = 0u;
        out_hdr->receiver_id = 0u;
        out_hdr->seq = 0u;
    }
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
    if (out_payload != NULL)
    {
        out_payload->current_update_id = 0u;
        out_payload->dataset_version_from = 0u;
        out_payload->dataset_version_to = 0u;
        out_payload->remove_count = 0u;
        out_payload->add_count = 0u;
        out_payload->update_payload_size = 0u;
        out_payload->update_payload_crc16 = 0u;
    }
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
    if (out_hdr->sender_id == 0u || out_hdr->receiver_id != 0u || out_payload->current_update_id == 0u || out_payload->dataset_version_to <= out_payload->dataset_version_from)
    {
        return 0;
    }
    if (out_payload->update_payload_size != 16u + 4u * (uint16_t)out_payload->remove_count + 4u * (uint16_t)out_payload->add_count)
    {
        return 0;
    }
    return 1;
}

int num8lora_decode_update_request(const uint8_t* buf, uint32_t len, num8lora_common_header_t* out_hdr, num8lora_update_request_payload_t* out_payload)
{
    if (out_payload != NULL)
    {
        out_payload->requested_update_id = 0u;
        out_payload->receiver_dataset_ver = 0u;
    }
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
    return out_hdr->sender_id != 0u && out_hdr->receiver_id != 0u && out_payload->requested_update_id != 0u;
}

int num8lora_decode_update_data_header(const uint8_t* buf, uint32_t len, num8lora_common_header_t* out_hdr, num8lora_update_header_t* out_upd_hdr, const uint8_t** out_remove_ptr, const uint8_t** out_add_ptr)
{
    uint32_t rc;
    uint32_t ac;
    uint32_t expected;
    if (out_upd_hdr != NULL)
    {
        out_upd_hdr->update_id = 0u;
        out_upd_hdr->dataset_version_from = 0u;
        out_upd_hdr->dataset_version_to = 0u;
        out_upd_hdr->remove_count = 0u;
        out_upd_hdr->add_count = 0u;
        out_upd_hdr->reserved0 = 0u;
    }
    if (out_remove_ptr != NULL)
    {
        *out_remove_ptr = NULL;
    }
    if (out_add_ptr != NULL)
    {
        *out_add_ptr = NULL;
    }
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
    if (len != expected || out_upd_hdr->reserved0 != 0u || out_hdr->sender_id == 0u || out_hdr->receiver_id == 0u || out_upd_hdr->update_id == 0u || out_upd_hdr->dataset_version_to <= out_upd_hdr->dataset_version_from)
    {
        return 0;
    }

    *out_remove_ptr = &buf[24];
    *out_add_ptr = &buf[24 + 4u * rc];
    return 1;
}

int num8lora_decode_ack(const uint8_t* buf, uint32_t len, num8lora_common_header_t* out_hdr, num8lora_ack_payload_t* out_payload)
{
    if (out_payload != NULL)
    {
        out_payload->ack_update_id = 0u;
        out_payload->receiver_dataset_ver = 0u;
    }
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
    return out_hdr->sender_id != 0u && out_hdr->receiver_id != 0u && out_payload->ack_update_id != 0u && out_payload->receiver_dataset_ver != 0u;
}

int num8lora_decode_nack(const uint8_t* buf, uint32_t len, num8lora_common_header_t* out_hdr, num8lora_nack_payload_t* out_payload)
{
    if (out_payload != NULL)
    {
        out_payload->nack_update_id = 0u;
        out_payload->error_code = 0u;
        out_payload->detail = 0u;
    }
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
    return out_hdr->sender_id != 0u && out_hdr->receiver_id != 0u && out_payload->nack_update_id != 0u;
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
    uint32_t remove_capacity,
    const uint8_t* add_ptr,
    uint32_t add_capacity,
    uint32_t* out_remove_numbers,
    uint32_t out_remove_capacity,
    uint32_t* out_add_numbers,
    uint32_t out_add_capacity,
    uint32_t* out_required_remove_count,
    uint32_t* out_required_add_count)
{
    uint32_t i;
    uint32_t rc;
    uint32_t ac;

    if (out_required_remove_count != NULL)
    {
        *out_required_remove_count = 0u;
    }
    if (out_required_add_count != NULL)
    {
        *out_required_add_count = 0u;
    }
    if (out_remove_numbers != NULL && out_remove_capacity > 0u)
    {
        for (i = 0u; i < out_remove_capacity; ++i)
        {
            out_remove_numbers[i] = 0u;
        }
    }
    if (out_add_numbers != NULL && out_add_capacity > 0u)
    {
        for (i = 0u; i < out_add_capacity; ++i)
        {
            out_add_numbers[i] = 0u;
        }
    }

    if (hdr == NULL || remove_ptr == NULL || add_ptr == NULL)
    {
        return 0;
    }

    rc = (uint32_t)hdr->remove_count;
    ac = (uint32_t)hdr->add_count;

    if (out_required_remove_count != NULL)
    {
        *out_required_remove_count = rc;
    }
    if (out_required_add_count != NULL)
    {
        *out_required_add_count = ac;
    }

    if (remove_capacity < rc || add_capacity < ac)
    {
        return 0;
    }

    if ((rc > 0u && out_remove_numbers == NULL) || (ac > 0u && out_add_numbers == NULL))
    {
        return 0;
    }

    if (out_remove_numbers != NULL && out_remove_capacity < rc)
    {
        return 0;
    }
    if (out_add_numbers != NULL && out_add_capacity < ac)
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
    uint32_t i;
    uint16_t crc = 0xFFFFu;

    if (out_crc16 != NULL)
    {
        *out_crc16 = 0u;
    }
    if (hdr == NULL || out_crc16 == NULL)
    {
        return 0;
    }
    if ((hdr->remove_count > 0u && remove_numbers == NULL) || (hdr->add_count > 0u && add_numbers == NULL))
    {
        return 0;
    }

    crc = crc16_update_u32(crc, hdr->update_id);
    crc = crc16_update_u32(crc, hdr->dataset_version_from);
    crc = crc16_update_u32(crc, hdr->dataset_version_to);
    crc = crc16_update_byte(crc, hdr->remove_count);
    crc = crc16_update_byte(crc, hdr->add_count);
    crc = crc16_update_u16(crc, 0u);

    for (i = 0; i < (uint32_t)hdr->remove_count; ++i)
    {
        crc = crc16_update_u32(crc, remove_numbers[i]);
    }
    for (i = 0; i < (uint32_t)hdr->add_count; ++i)
    {
        crc = crc16_update_u32(crc, add_numbers[i]);
    }

    *out_crc16 = crc;
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
    if (out_hdr != NULL)
    {
        out_hdr->protocol_version = 0u;
        out_hdr->msg_type = 0u;
        out_hdr->sender_id = 0u;
        out_hdr->receiver_id = 0u;
        out_hdr->seq = 0u;
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
