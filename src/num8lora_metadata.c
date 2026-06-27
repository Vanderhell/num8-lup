#include "num8lora_metadata.h"

#include <string.h>

#define NUM8LORA_METADATA_MAGIC0 'N'
#define NUM8LORA_METADATA_MAGIC1 '8'
#define NUM8LORA_METADATA_MAGIC2 'A'
#define NUM8LORA_METADATA_MAGIC3 'M'

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

static uint16_t crc16_update(uint16_t crc, uint8_t byte)
{
    uint32_t i;

    crc ^= (uint16_t)((uint16_t)byte << 8);
    for (i = 0u; i < 8u; ++i)
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
    crc = crc16_update(crc, (uint8_t)(v & 0xFFu));
    return crc16_update(crc, (uint8_t)((v >> 8) & 0xFFu));
}

static uint16_t crc16_update_u32(uint16_t crc, uint32_t v)
{
    crc = crc16_update(crc, (uint8_t)(v & 0xFFu));
    crc = crc16_update(crc, (uint8_t)((v >> 8) & 0xFFu));
    crc = crc16_update(crc, (uint8_t)((v >> 16) & 0xFFu));
    return crc16_update(crc, (uint8_t)((v >> 24) & 0xFFu));
}

uint32_t num8lora_metadata_record_size(void)
{
    return 26u;
}

num8lora_metadata_status_t num8lora_metadata_encode_record(
    const num8lora_metadata_record_t* record,
    uint8_t* out_buf,
    uint32_t out_cap,
    uint32_t* out_len,
    uint16_t* out_error_code)
{
    uint16_t crc = 0xFFFFu;

    if (out_len != NULL)
    {
        *out_len = 0u;
    }
    if (out_error_code != NULL)
    {
        *out_error_code = NUM8LORA_METADATA_ERR_NONE;
    }
    if (record == NULL || out_buf == NULL || out_len == NULL)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_METADATA_ERR_INTERNAL;
        }
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }
    if (record->format_version != NUM8LORA_METADATA_FORMAT_VERSION)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_METADATA_ERR_VERSION;
        }
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }
    if (record->reserved0 != 0u)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_METADATA_ERR_RESERVED;
        }
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }
    if (out_cap < num8lora_metadata_record_size())
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_METADATA_ERR_LENGTH;
        }
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }

    out_buf[0] = NUM8LORA_METADATA_MAGIC0;
    out_buf[1] = NUM8LORA_METADATA_MAGIC1;
    out_buf[2] = NUM8LORA_METADATA_MAGIC2;
    out_buf[3] = NUM8LORA_METADATA_MAGIC3;
    st32(&out_buf[4], record->format_version);
    st32(&out_buf[8], record->reserved0);
    st32(&out_buf[12], record->stream_id);
    st32(&out_buf[16], record->stream_epoch);
    st32(&out_buf[20], record->last_applied_op_id);

    crc = crc16_update_u32(crc, record->format_version);
    crc = crc16_update_u32(crc, record->reserved0);
    crc = crc16_update_u32(crc, record->stream_id);
    crc = crc16_update_u32(crc, record->stream_epoch);
    crc = crc16_update_u32(crc, record->last_applied_op_id);
    crc = crc16_update_u16(crc, 0u);
    st16(&out_buf[24], crc);

    *out_len = num8lora_metadata_record_size();
    return NUM8LORA_METADATA_STATUS_SUCCESS;
}

num8lora_metadata_status_t num8lora_metadata_decode_record(
    const uint8_t* buf,
    uint32_t len,
    num8lora_metadata_record_t* out_record,
    uint16_t* out_error_code)
{
    uint16_t crc;

    if (out_record != NULL)
    {
        memset(out_record, 0, sizeof(*out_record));
    }
    if (out_error_code != NULL)
    {
        *out_error_code = NUM8LORA_METADATA_ERR_NONE;
    }
    if (buf == NULL || out_record == NULL)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_METADATA_ERR_INTERNAL;
        }
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }
    if (len != num8lora_metadata_record_size())
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_METADATA_ERR_LENGTH;
        }
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }
    if (buf[0] != NUM8LORA_METADATA_MAGIC0 || buf[1] != NUM8LORA_METADATA_MAGIC1 || buf[2] != NUM8LORA_METADATA_MAGIC2 || buf[3] != NUM8LORA_METADATA_MAGIC3)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_METADATA_ERR_MAGIC;
        }
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }

    out_record->format_version = ld32(&buf[4]);
    out_record->reserved0 = ld32(&buf[8]);
    out_record->stream_id = ld32(&buf[12]);
    out_record->stream_epoch = ld32(&buf[16]);
    out_record->last_applied_op_id = ld32(&buf[20]);

    if (out_record->format_version != NUM8LORA_METADATA_FORMAT_VERSION)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_METADATA_ERR_VERSION;
        }
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }
    if (out_record->reserved0 != 0u)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_METADATA_ERR_RESERVED;
        }
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }

    crc = crc16_update_u32(0xFFFFu, out_record->format_version);
    crc = crc16_update_u32(crc, out_record->reserved0);
    crc = crc16_update_u32(crc, out_record->stream_id);
    crc = crc16_update_u32(crc, out_record->stream_epoch);
    crc = crc16_update_u32(crc, out_record->last_applied_op_id);
    crc = crc16_update_u16(crc, 0u);
    if (crc != ld16(&buf[24]))
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_METADATA_ERR_CRC;
        }
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }

    return NUM8LORA_METADATA_STATUS_SUCCESS;
}

num8lora_metadata_status_t num8lora_metadata_record_matches_stream(
    const num8lora_metadata_record_t* record,
    uint32_t stream_id,
    uint32_t stream_epoch,
    uint8_t* out_matches)
{
    if (out_matches != NULL)
    {
        *out_matches = 0u;
    }
    if (record == NULL || out_matches == NULL)
    {
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }

    *out_matches = (uint8_t)((record->format_version == NUM8LORA_METADATA_FORMAT_VERSION
        && record->stream_id == stream_id
        && record->stream_epoch == stream_epoch) ? 1u : 0u);
    return NUM8LORA_METADATA_STATUS_SUCCESS;
}
