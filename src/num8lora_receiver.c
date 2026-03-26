#include "num8lora_receiver.h"

#define MAX_VALUE 99999999u

void num8lora_receiver_init(num8lora_receiver_t* r, uint16_t receiver_id, uint16_t expected_sender_id, uint32_t stream_id, uint32_t last_applied_op_id)
{
    if (r == NULL)
    {
        return;
    }
    r->receiver_id = receiver_id;
    r->expected_sender_id = expected_sender_id;
    r->seq = 1u;
    r->stream_id = stream_id;
    r->last_applied_op_id = last_applied_op_id;
}

int num8lora_receiver_handle_beacon(
    num8lora_receiver_t* r,
    const uint8_t* in_buf,
    uint32_t in_len,
    uint8_t* out_buf,
    uint32_t out_cap,
    uint32_t* out_len)
{
    num8lora_op_common_header_t hdr;
    num8lora_op_beacon_payload_t b;
    num8lora_op_request_payload_t req;

    if (r == NULL || in_buf == NULL || out_buf == NULL || out_len == NULL)
    {
        return 0;
    }
    if (!num8lora_op_decode_beacon(in_buf, in_len, &hdr, &b))
    {
        return 0;
    }
    if (r->expected_sender_id != 0u && hdr.sender_id != r->expected_sender_id)
    {
        return 0;
    }
    if (b.stream_id != r->stream_id)
    {
        return 0;
    }
    if (b.latest_op_id <= r->last_applied_op_id)
    {
        return 0;
    }

    req.stream_id = r->stream_id;
    req.next_needed_op_id = r->last_applied_op_id + 1u;
    return num8lora_op_encode_request(out_buf, out_cap, r->receiver_id, hdr.sender_id, r->seq++, &req, out_len);
}

int num8lora_receiver_handle_data(
    num8lora_receiver_t* r,
    const uint8_t* in_buf,
    uint32_t in_len,
    num8lora_receiver_apply_fn apply_fn,
    void* apply_user,
    uint8_t* out_buf,
    uint32_t out_cap,
    uint32_t* out_len)
{
    num8lora_op_common_header_t hdr;
    num8lora_op_data_payload_t d;
    num8lora_op_ack_payload_t ack;
    num8lora_op_nack_payload_t nack;

    if (r == NULL || in_buf == NULL || out_buf == NULL || out_len == NULL)
    {
        return 0;
    }

    if (!num8lora_op_decode_data(in_buf, in_len, &hdr, &d))
    {
        return 0;
    }

    if (r->expected_sender_id != 0u && hdr.sender_id != r->expected_sender_id)
    {
        nack.stream_id = r->stream_id;
        nack.error_code = NUM8LORA_OP_ERR_RECEIVER;
        nack.detail = 0u;
        nack.expected_next_op_id = r->last_applied_op_id + 1u;
        return num8lora_op_encode_nack(out_buf, out_cap, r->receiver_id, hdr.sender_id, r->seq++, &nack, out_len);
    }
    if (hdr.receiver_id != r->receiver_id)
    {
        return 0;
    }
    if (d.stream_id != r->stream_id)
    {
        nack.stream_id = r->stream_id;
        nack.error_code = NUM8LORA_OP_ERR_STREAM;
        nack.detail = 0u;
        nack.expected_next_op_id = r->last_applied_op_id + 1u;
        return num8lora_op_encode_nack(out_buf, out_cap, r->receiver_id, hdr.sender_id, r->seq++, &nack, out_len);
    }
    if (d.value > MAX_VALUE || (d.op_type != NUM8LORA_OP_ADD && d.op_type != NUM8LORA_OP_REMOVE))
    {
        nack.stream_id = r->stream_id;
        nack.error_code = NUM8LORA_OP_ERR_INVALID_VALUE;
        nack.detail = 0u;
        nack.expected_next_op_id = r->last_applied_op_id + 1u;
        return num8lora_op_encode_nack(out_buf, out_cap, r->receiver_id, hdr.sender_id, r->seq++, &nack, out_len);
    }

    if (d.op_id <= r->last_applied_op_id)
    {
        ack.stream_id = r->stream_id;
        ack.ack_op_id = r->last_applied_op_id;
        return num8lora_op_encode_ack(out_buf, out_cap, r->receiver_id, hdr.sender_id, r->seq++, &ack, out_len);
    }

    if (d.op_id != r->last_applied_op_id + 1u)
    {
        nack.stream_id = r->stream_id;
        nack.error_code = NUM8LORA_OP_ERR_SEQUENCE_GAP;
        nack.detail = 0u;
        nack.expected_next_op_id = r->last_applied_op_id + 1u;
        return num8lora_op_encode_nack(out_buf, out_cap, r->receiver_id, hdr.sender_id, r->seq++, &nack, out_len);
    }

    if (apply_fn != NULL)
    {
        if (!apply_fn(apply_user, d.op_type, d.value))
        {
            nack.stream_id = r->stream_id;
            nack.error_code = NUM8LORA_OP_ERR_APPLY_FAILED;
            nack.detail = 0u;
            nack.expected_next_op_id = r->last_applied_op_id + 1u;
            return num8lora_op_encode_nack(out_buf, out_cap, r->receiver_id, hdr.sender_id, r->seq++, &nack, out_len);
        }
    }

    r->last_applied_op_id = d.op_id;

    ack.stream_id = r->stream_id;
    ack.ack_op_id = r->last_applied_op_id;
    return num8lora_op_encode_ack(out_buf, out_cap, r->receiver_id, hdr.sender_id, r->seq++, &ack, out_len);
}

#ifdef NUM8LORA_ENABLE_NUM8
int num8lora_receiver_apply_num8(void* user, uint8_t op_type, uint32_t value)
{
    num8_engine_t* e = (num8_engine_t*)user;
    num8_status_t st;

    if (e == NULL)
    {
        return 0;
    }

    if (op_type == NUM8LORA_OP_ADD)
    {
        st = num8_add_u32(e, value);
        return st == NUM8_STATUS_ADDED || st == NUM8_STATUS_ALREADY_EXISTS;
    }
    if (op_type == NUM8LORA_OP_REMOVE)
    {
        st = num8_remove_u32(e, value);
        return st == NUM8_STATUS_REMOVED || st == NUM8_STATUS_NOT_FOUND;
    }
    return 0;
}
#endif
