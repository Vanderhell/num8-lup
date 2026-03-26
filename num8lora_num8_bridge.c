#include "num8lora.h"
#include "num8.h"

int num8lora_apply_update_to_num8(
    num8_engine_t* engine,
    uint32_t local_dataset_version,
    uint32_t last_applied_update_id,
    const num8lora_update_header_t* hdr,
    const uint32_t* remove_numbers,
    const uint32_t* add_numbers,
    uint32_t* out_new_dataset_version,
    uint32_t* out_new_last_applied_update_id,
    uint16_t* out_error_code)
{
    uint32_t i;
    uint16_t err = NUM8LORA_ERR_NONE;
    if (out_error_code != 0)
    {
        *out_error_code = NUM8LORA_ERR_NONE;
    }
    if (out_new_dataset_version == 0 || out_new_last_applied_update_id == 0 || engine == 0 || hdr == 0)
    {
        if (out_error_code != 0)
        {
            *out_error_code = NUM8LORA_ERR_INTERNAL;
        }
        return 0;
    }

    if (hdr->dataset_version_from != local_dataset_version)
    {
        if (out_error_code != 0)
        {
            *out_error_code = NUM8LORA_ERR_DATASET_VERSION_MISMATCH;
        }
        return 0;
    }

    if (hdr->dataset_version_to <= hdr->dataset_version_from)
    {
        if (out_error_code != 0)
        {
            *out_error_code = NUM8LORA_ERR_DATASET_VERSION_MISMATCH;
        }
        return 0;
    }

    if (hdr->update_id == last_applied_update_id)
    {
        *out_new_dataset_version = local_dataset_version;
        *out_new_last_applied_update_id = last_applied_update_id;
        return 1;
    }

    if (!num8lora_validate_update_numbers(hdr, remove_numbers, add_numbers, &err))
    {
        if (out_error_code != 0)
        {
            *out_error_code = err;
        }
        return 0;
    }

    for (i = 0; i < (uint32_t)hdr->remove_count; ++i)
    {
        num8_status_t st = num8_remove_u32(engine, remove_numbers[i]);
        if (st != NUM8_STATUS_REMOVED && st != NUM8_STATUS_NOT_FOUND)
        {
            if (out_error_code != 0)
            {
                *out_error_code = NUM8LORA_ERR_APPLY_FAILED;
            }
            return 0;
        }
    }

    for (i = 0; i < (uint32_t)hdr->add_count; ++i)
    {
        num8_status_t st = num8_add_u32(engine, add_numbers[i]);
        if (st != NUM8_STATUS_ADDED && st != NUM8_STATUS_ALREADY_EXISTS)
        {
            if (out_error_code != 0)
            {
                *out_error_code = NUM8LORA_ERR_APPLY_FAILED;
            }
            return 0;
        }
    }

    if (num8_flush(engine) != NUM8_STATUS_OK)
    {
        if (out_error_code != 0)
        {
            *out_error_code = NUM8LORA_ERR_APPLY_FAILED;
        }
        return 0;
    }

    *out_new_dataset_version = hdr->dataset_version_to;
    *out_new_last_applied_update_id = hdr->update_id;
    return 1;
}

int num8lora_receiver_apply_update_data_to_num8(
    num8lora_receiver_ctx_t* ctx,
    num8_engine_t* engine,
    const uint8_t* update_buf,
    uint32_t update_len,
    uint8_t* out_response_buf,
    uint32_t out_cap,
    uint32_t* out_response_len,
    uint16_t* out_error_code)
{
    num8lora_common_header_t hdr;
    num8lora_update_header_t up;
    uint16_t err = NUM8LORA_ERR_NONE;
    uint32_t remove_numbers[255];
    uint32_t add_numbers[255];
    uint32_t new_version = 0;
    uint32_t new_last = 0;

    if (out_error_code != 0)
    {
        *out_error_code = NUM8LORA_ERR_NONE;
    }

    if (ctx == 0 || engine == 0 || update_buf == 0 || out_response_buf == 0 || out_response_len == 0)
    {
        if (out_error_code != 0)
        {
            *out_error_code = NUM8LORA_ERR_INTERNAL;
        }
        return 0;
    }

    if (!num8lora_decode_common_header(update_buf, update_len, &hdr))
    {
        err = NUM8LORA_ERR_INTERNAL;
        if (out_error_code != 0)
        {
            *out_error_code = err;
        }
        return 0;
    }

    if (!num8lora_receiver_validate_update_data(ctx, update_buf, update_len, &up, remove_numbers, add_numbers, &err))
    {
        if (!num8lora_receiver_encode_nack(ctx, hdr.sender_id, ctx->pending_update_id, err, 0u, out_response_buf, out_cap, out_response_len))
        {
            if (out_error_code != 0)
            {
                *out_error_code = NUM8LORA_ERR_INTERNAL;
            }
            return 0;
        }
        if (out_error_code != 0)
        {
            *out_error_code = err;
        }
        return 1;
    }

    if (up.update_id == ctx->last_applied_update_id)
    {
        if (!num8lora_receiver_encode_ack(ctx, hdr.sender_id, up.update_id, out_response_buf, out_cap, out_response_len))
        {
            if (out_error_code != 0)
            {
                *out_error_code = NUM8LORA_ERR_INTERNAL;
            }
            return 0;
        }
        return 1;
    }

    if (!num8lora_apply_update_to_num8(
        engine,
        ctx->local_dataset_version,
        ctx->last_applied_update_id,
        &up,
        remove_numbers,
        add_numbers,
        &new_version,
        &new_last,
        &err))
    {
        if (!num8lora_receiver_encode_nack(ctx, hdr.sender_id, up.update_id, err, 0u, out_response_buf, out_cap, out_response_len))
        {
            if (out_error_code != 0)
            {
                *out_error_code = NUM8LORA_ERR_INTERNAL;
            }
            return 0;
        }
        if (out_error_code != 0)
        {
            *out_error_code = err;
        }
        return 1;
    }

    ctx->local_dataset_version = new_version;
    ctx->last_applied_update_id = new_last;
    ctx->state = NUM8LORA_RECEIVER_SCAN;

    if (!num8lora_receiver_encode_ack(ctx, hdr.sender_id, up.update_id, out_response_buf, out_cap, out_response_len))
    {
        if (out_error_code != 0)
        {
            *out_error_code = NUM8LORA_ERR_INTERNAL;
        }
        return 0;
    }

    if (out_error_code != 0)
    {
        *out_error_code = NUM8LORA_ERR_NONE;
    }
    return 1;
}
