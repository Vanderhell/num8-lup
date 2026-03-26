#include "num8lora.h"

#include <string.h>

static uint32_t load_u32_le(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

void num8lora_sender_init(num8lora_sender_ctx_t* ctx, uint16_t sender_id, uint8_t max_retries)
{
    if (ctx == NULL)
    {
        return;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->state = NUM8LORA_SENDER_IDLE;
    ctx->sender_id = sender_id;
    ctx->max_retries = max_retries;
    ctx->seq = 1u;
}

int num8lora_sender_set_update(
    num8lora_sender_ctx_t* ctx,
    uint16_t target_receiver_id,
    const num8lora_update_header_t* hdr,
    const uint32_t* remove_numbers,
    const uint32_t* add_numbers,
    uint16_t* out_error_code)
{
    uint16_t err = NUM8LORA_ERR_NONE;

    if (out_error_code != NULL)
    {
        *out_error_code = NUM8LORA_ERR_NONE;
    }
    if (ctx == NULL || hdr == NULL)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_ERR_INTERNAL;
        }
        return 0;
    }

    if (ctx->state == NUM8LORA_SENDER_WAIT_ACK)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_ERR_BUSY;
        }
        return 0;
    }

    if (hdr->dataset_version_to <= hdr->dataset_version_from || hdr->reserved0 != 0u)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_ERR_DATASET_VERSION_MISMATCH;
        }
        return 0;
    }

    if (!num8lora_validate_update_numbers(hdr, remove_numbers, add_numbers, &err))
    {
        if (out_error_code != NULL)
        {
            *out_error_code = err;
        }
        return 0;
    }

    if (!num8lora_compute_update_payload_crc16(hdr, remove_numbers, add_numbers, &ctx->update_payload_crc16))
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_ERR_INTERNAL;
        }
        return 0;
    }

    ctx->target_receiver_id = target_receiver_id;
    ctx->update_hdr = *hdr;
    ctx->remove_numbers = remove_numbers;
    ctx->add_numbers = add_numbers;
    ctx->retries = 0u;
    ctx->last_nack_error = NUM8LORA_ERR_NONE;
    ctx->state = NUM8LORA_SENDER_WAIT_REQUEST;
    return 1;
}

int num8lora_sender_build_beacon(num8lora_sender_ctx_t* ctx, uint8_t* out_buf, uint32_t out_cap, uint32_t* out_len)
{
    num8lora_beacon_payload_t p;

    if (ctx == NULL || out_buf == NULL || out_len == NULL)
    {
        return 0;
    }

    p.current_update_id = ctx->update_hdr.update_id;
    p.dataset_version_from = ctx->update_hdr.dataset_version_from;
    p.dataset_version_to = ctx->update_hdr.dataset_version_to;
    p.remove_count = ctx->update_hdr.remove_count;
    p.add_count = ctx->update_hdr.add_count;
    p.update_payload_size = (uint16_t)(16u + 4u * (uint16_t)p.remove_count + 4u * (uint16_t)p.add_count);
    p.update_payload_crc16 = ctx->update_payload_crc16;

    return num8lora_encode_beacon(out_buf, out_cap, ctx->sender_id, ctx->seq++, &p, out_len);
}

static int sender_emit_update(const num8lora_sender_ctx_t* ctx, uint16_t receiver_id, uint16_t seq, uint8_t* out_response_buf, uint32_t out_cap, uint32_t* out_len)
{
    return num8lora_encode_update_data(
        out_response_buf,
        out_cap,
        ctx->sender_id,
        receiver_id,
        seq,
        &ctx->update_hdr,
        ctx->remove_numbers,
        ctx->add_numbers,
        out_len);
}

int num8lora_sender_handle_update_request(
    num8lora_sender_ctx_t* ctx,
    const uint8_t* req_buf,
    uint32_t req_len,
    uint8_t* out_response_buf,
    uint32_t out_cap,
    uint32_t* out_len)
{
    num8lora_common_header_t hdr;
    num8lora_update_request_payload_t req;
    uint16_t err = NUM8LORA_ERR_NONE;

    if (ctx == NULL || req_buf == NULL || out_response_buf == NULL || out_len == NULL)
    {
        return 0;
    }
    if (ctx->state != NUM8LORA_SENDER_WAIT_REQUEST && ctx->state != NUM8LORA_SENDER_WAIT_ACK)
    {
        return 0;
    }

    if (!num8lora_classify_frame(req_buf, req_len, ctx->sender_id, NUM8LORA_MSG_UPDATE_REQUEST, &hdr, &err)
        || !num8lora_decode_update_request(req_buf, req_len, &hdr, &req))
    {
        num8lora_nack_payload_t nack;
        nack.nack_update_id = ctx->update_hdr.update_id;
        nack.error_code = err == NUM8LORA_ERR_NONE ? NUM8LORA_ERR_INTERNAL : err;
        nack.detail = 0u;
        ctx->last_nack_error = nack.error_code;
        ctx->state = NUM8LORA_SENDER_IDLE;
        return num8lora_encode_nack(out_response_buf, out_cap, ctx->sender_id, hdr.sender_id, ctx->seq++, &nack, out_len);
    }

    if (req.requested_update_id != ctx->update_hdr.update_id)
    {
        num8lora_nack_payload_t nack;
        nack.nack_update_id = req.requested_update_id;
        nack.error_code = NUM8LORA_ERR_UPDATE_ID_UNKNOWN;
        nack.detail = 0u;
        ctx->last_nack_error = nack.error_code;
        ctx->state = NUM8LORA_SENDER_IDLE;
        return num8lora_encode_nack(out_response_buf, out_cap, ctx->sender_id, hdr.sender_id, ctx->seq++, &nack, out_len);
    }

    if (req.receiver_dataset_ver != ctx->update_hdr.dataset_version_from)
    {
        num8lora_nack_payload_t nack;
        nack.nack_update_id = req.requested_update_id;
        nack.error_code = NUM8LORA_ERR_DATASET_VERSION_MISMATCH;
        nack.detail = 0u;
        ctx->last_nack_error = nack.error_code;
        ctx->state = NUM8LORA_SENDER_IDLE;
        return num8lora_encode_nack(out_response_buf, out_cap, ctx->sender_id, hdr.sender_id, ctx->seq++, &nack, out_len);
    }

    ctx->target_receiver_id = hdr.sender_id;
    ctx->state = NUM8LORA_SENDER_WAIT_ACK;
    ctx->retries = 0u;
    return sender_emit_update(ctx, hdr.sender_id, ctx->seq++, out_response_buf, out_cap, out_len);
}

int num8lora_sender_handle_ack_or_nack(
    num8lora_sender_ctx_t* ctx,
    const uint8_t* in_buf,
    uint32_t in_len,
    num8lora_sender_ack_result_t* out_result,
    uint16_t* out_nack_error_code)
{
    num8lora_common_header_t hdr;
    uint16_t err = NUM8LORA_ERR_NONE;

    if (out_result != NULL)
    {
        *out_result = NUM8LORA_ACKRES_NONE;
    }
    if (out_nack_error_code != NULL)
    {
        *out_nack_error_code = NUM8LORA_ERR_NONE;
    }

    if (ctx == NULL || in_buf == NULL || out_result == NULL)
    {
        return 0;
    }
    if (ctx->state != NUM8LORA_SENDER_WAIT_ACK)
    {
        return 0;
    }

    if (!num8lora_classify_frame(in_buf, in_len, ctx->sender_id, 0u, &hdr, &err))
    {
        return 0;
    }

    if (hdr.msg_type == NUM8LORA_MSG_ACK)
    {
        num8lora_ack_payload_t ack;
        if (!num8lora_decode_ack(in_buf, in_len, &hdr, &ack))
        {
            return 0;
        }
        if (ack.ack_update_id == ctx->update_hdr.update_id && ack.receiver_dataset_ver == ctx->update_hdr.dataset_version_to)
        {
            ctx->state = NUM8LORA_SENDER_IDLE;
            ctx->retries = 0u;
            *out_result = NUM8LORA_ACKRES_APPLIED;
            return 1;
        }
        return 0;
    }

    if (hdr.msg_type == NUM8LORA_MSG_NACK)
    {
        num8lora_nack_payload_t nack;
        if (!num8lora_decode_nack(in_buf, in_len, &hdr, &nack))
        {
            return 0;
        }
        if (out_nack_error_code != NULL)
        {
            *out_nack_error_code = nack.error_code;
        }
        ctx->last_nack_error = nack.error_code;
        ctx->state = NUM8LORA_SENDER_IDLE;
        *out_result = NUM8LORA_ACKRES_NACKED;
        return 1;
    }

    return 0;
}

int num8lora_sender_on_ack_timeout(
    num8lora_sender_ctx_t* ctx,
    uint8_t* out_update_buf,
    uint32_t out_cap,
    uint32_t* out_len)
{
    if (ctx == NULL || out_update_buf == NULL || out_len == NULL)
    {
        return 0;
    }
    if (ctx->state != NUM8LORA_SENDER_WAIT_ACK)
    {
        return 0;
    }
    if (ctx->retries >= ctx->max_retries)
    {
        ctx->state = NUM8LORA_SENDER_IDLE;
        return 0;
    }

    ctx->retries++;
    return sender_emit_update(ctx, ctx->target_receiver_id, ctx->seq++, out_update_buf, out_cap, out_len);
}

void num8lora_receiver_init(
    num8lora_receiver_ctx_t* ctx,
    uint16_t receiver_id,
    uint16_t expected_sender_id,
    uint8_t max_retries,
    uint32_t local_dataset_version,
    uint32_t last_applied_update_id)
{
    if (ctx == NULL)
    {
        return;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->state = NUM8LORA_RECEIVER_SCAN;
    ctx->receiver_id = receiver_id;
    ctx->expected_sender_id = expected_sender_id;
    ctx->max_retries = max_retries;
    ctx->local_dataset_version = local_dataset_version;
    ctx->last_applied_update_id = last_applied_update_id;
    ctx->seq = 1u;
}

int num8lora_receiver_handle_beacon(
    num8lora_receiver_ctx_t* ctx,
    const uint8_t* beacon_buf,
    uint32_t beacon_len,
    uint8_t* out_request_buf,
    uint32_t out_cap,
    uint32_t* out_len)
{
    num8lora_common_header_t hdr;
    num8lora_beacon_payload_t p;
    uint16_t err = NUM8LORA_ERR_NONE;
    num8lora_update_request_payload_t req;

    if (ctx == NULL || beacon_buf == NULL || out_request_buf == NULL || out_len == NULL)
    {
        return 0;
    }

    if (!num8lora_classify_frame(beacon_buf, beacon_len, NUM8LORA_ANY_RECEIVER_ID, NUM8LORA_MSG_BEACON, &hdr, &err)
        || !num8lora_decode_beacon(beacon_buf, beacon_len, &hdr, &p))
    {
        return 0;
    }

    if (ctx->expected_sender_id != 0u && hdr.sender_id != ctx->expected_sender_id)
    {
        return 0;
    }

    if (p.dataset_version_to <= ctx->local_dataset_version)
    {
        return 0;
    }

    ctx->pending_update_id = p.current_update_id;
    ctx->pending_dataset_version_from = p.dataset_version_from;
    ctx->pending_dataset_version_to = p.dataset_version_to;
    ctx->pending_remove_count = p.remove_count;
    ctx->pending_add_count = p.add_count;
    ctx->pending_payload_size = p.update_payload_size;
    ctx->pending_payload_crc16 = p.update_payload_crc16;
    ctx->state = NUM8LORA_RECEIVER_WAIT_UPDATE;
    ctx->retries = 0u;

    req.requested_update_id = p.current_update_id;
    req.receiver_dataset_ver = ctx->local_dataset_version;
    return num8lora_encode_update_request(out_request_buf, out_cap, ctx->receiver_id, hdr.sender_id, ctx->seq++, &req, out_len);
}

int num8lora_receiver_on_update_timeout(
    num8lora_receiver_ctx_t* ctx,
    uint8_t* out_request_buf,
    uint32_t out_cap,
    uint32_t* out_len)
{
    num8lora_update_request_payload_t req;

    if (ctx == NULL || out_request_buf == NULL || out_len == NULL)
    {
        return 0;
    }
    if (ctx->state != NUM8LORA_RECEIVER_WAIT_UPDATE)
    {
        return 0;
    }
    if (ctx->retries >= ctx->max_retries)
    {
        ctx->state = NUM8LORA_RECEIVER_SCAN;
        return 0;
    }

    ctx->retries++;
    req.requested_update_id = ctx->pending_update_id;
    req.receiver_dataset_ver = ctx->local_dataset_version;
    return num8lora_encode_update_request(out_request_buf, out_cap, ctx->receiver_id, ctx->expected_sender_id, ctx->seq++, &req, out_len);
}

int num8lora_receiver_validate_update_data(
    num8lora_receiver_ctx_t* ctx,
    const uint8_t* update_buf,
    uint32_t update_len,
    num8lora_update_header_t* out_hdr,
    uint32_t* out_remove_numbers,
    uint32_t* out_add_numbers,
    uint16_t* out_error_code)
{
    num8lora_common_header_t hdr;
    num8lora_update_header_t up;
    const uint8_t* rp;
    const uint8_t* ap;
    uint16_t err = NUM8LORA_ERR_NONE;
    uint16_t payload_crc = 0;

    if (out_error_code != NULL)
    {
        *out_error_code = NUM8LORA_ERR_NONE;
    }
    if (ctx == NULL || update_buf == NULL || out_hdr == NULL)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_ERR_INTERNAL;
        }
        return 0;
    }

    if (!num8lora_classify_frame(update_buf, update_len, ctx->receiver_id, NUM8LORA_MSG_UPDATE_DATA, &hdr, &err))
    {
        if (out_error_code != NULL)
        {
            *out_error_code = err;
        }
        return 0;
    }

    if (ctx->expected_sender_id != 0u && hdr.sender_id != ctx->expected_sender_id)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_ERR_INVALID_RECEIVER_ID;
        }
        return 0;
    }

    if (!num8lora_decode_update_data_header(update_buf, update_len, &hdr, &up, &rp, &ap))
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_ERR_INTERNAL;
        }
        return 0;
    }

    if (up.update_id == ctx->last_applied_update_id)
    {
        *out_hdr = up;
        return 1;
    }

    if (ctx->state != NUM8LORA_RECEIVER_WAIT_UPDATE)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_ERR_UPDATE_ID_UNKNOWN;
        }
        return 0;
    }

    if (up.update_id != ctx->pending_update_id)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_ERR_UPDATE_ID_UNKNOWN;
        }
        return 0;
    }

    if (up.dataset_version_from != ctx->local_dataset_version || up.dataset_version_to <= up.dataset_version_from)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_ERR_DATASET_VERSION_MISMATCH;
        }
        return 0;
    }

    if ((uint16_t)(16u + 4u * up.remove_count + 4u * up.add_count) != ctx->pending_payload_size)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_ERR_INTERNAL;
        }
        return 0;
    }

    if (!num8lora_decode_update_numbers(&up, rp, ap, out_remove_numbers, out_add_numbers))
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_ERR_INTERNAL;
        }
        return 0;
    }

    if (!num8lora_validate_update_numbers(&up, out_remove_numbers, out_add_numbers, &err))
    {
        if (out_error_code != NULL)
        {
            *out_error_code = err;
        }
        return 0;
    }

    if (!num8lora_compute_update_payload_crc16(&up, out_remove_numbers, out_add_numbers, &payload_crc))
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_ERR_INTERNAL;
        }
        return 0;
    }

    if (ctx->pending_payload_crc16 != 0u && payload_crc != ctx->pending_payload_crc16)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_ERR_CRC_FAIL;
        }
        return 0;
    }

    *out_hdr = up;
    return 1;
}

int num8lora_receiver_encode_ack(
    num8lora_receiver_ctx_t* ctx,
    uint16_t sender_id,
    uint32_t ack_update_id,
    uint8_t* out_ack_buf,
    uint32_t out_cap,
    uint32_t* out_len)
{
    num8lora_ack_payload_t p;
    if (ctx == NULL || out_ack_buf == NULL || out_len == NULL)
    {
        return 0;
    }
    p.ack_update_id = ack_update_id;
    p.receiver_dataset_ver = ctx->local_dataset_version;
    return num8lora_encode_ack(out_ack_buf, out_cap, ctx->receiver_id, sender_id, ctx->seq++, &p, out_len);
}

int num8lora_receiver_encode_nack(
    num8lora_receiver_ctx_t* ctx,
    uint16_t sender_id,
    uint32_t nack_update_id,
    uint16_t error_code,
    uint16_t detail,
    uint8_t* out_nack_buf,
    uint32_t out_cap,
    uint32_t* out_len)
{
    num8lora_nack_payload_t p;
    if (ctx == NULL || out_nack_buf == NULL || out_len == NULL)
    {
        return 0;
    }
    p.nack_update_id = nack_update_id;
    p.error_code = error_code;
    p.detail = detail;
    return num8lora_encode_nack(out_nack_buf, out_cap, ctx->receiver_id, sender_id, ctx->seq++, &p, out_len);
}
