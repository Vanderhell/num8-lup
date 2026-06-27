#include "num8lora_metadata.h"
#include "num8lora_codec.h"

#include <string.h>

#define NUM8LORA_METADATA_MAGIC0 'N'
#define NUM8LORA_METADATA_MAGIC1 '8'
#define NUM8LORA_METADATA_MAGIC2 'A'
#define NUM8LORA_METADATA_MAGIC3 'M'

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
    num8lora_codec_write_u32_le(&out_buf[4], record->format_version);
    num8lora_codec_write_u32_le(&out_buf[8], record->reserved0);
    num8lora_codec_write_u32_le(&out_buf[12], record->stream_id);
    num8lora_codec_write_u32_le(&out_buf[16], record->stream_epoch);
    num8lora_codec_write_u32_le(&out_buf[20], record->last_applied_op_id);

    crc = num8lora_codec_crc16_ccitt_false(&out_buf[4], 20u);
    num8lora_codec_write_u16_le(&out_buf[24], crc);

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

    out_record->format_version = num8lora_codec_read_u32_le(&buf[4]);
    out_record->reserved0 = num8lora_codec_read_u32_le(&buf[8]);
    out_record->stream_id = num8lora_codec_read_u32_le(&buf[12]);
    out_record->stream_epoch = num8lora_codec_read_u32_le(&buf[16]);
    out_record->last_applied_op_id = num8lora_codec_read_u32_le(&buf[20]);

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

    crc = num8lora_codec_crc16_ccitt_false(&buf[4], 20u);
    if (crc != num8lora_codec_read_u16_le(&buf[24]))
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
