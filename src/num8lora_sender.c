#include "num8lora_sender.h"

#include <string.h>

#define MAX_VALUE 99999999u

static num8lora_sender_receiver_slot_t* find_slot(num8lora_sender_t* s, uint16_t rid)
{
    uint32_t i;
    for (i = 0; i < s->receiver_capacity; ++i)
    {
        if (s->receivers[i].active && s->receivers[i].receiver_id == rid)
        {
            return &s->receivers[i];
        }
    }
    return NULL;
}

static const num8lora_sender_op_t* find_op(const num8lora_sender_t* s, uint32_t op_id)
{
    if (op_id == 0u || op_id > s->op_count)
    {
        return NULL;
    }
    return &s->ops[op_id - 1u];
}

static int emit_data(num8lora_sender_t* s, num8lora_sender_receiver_slot_t* slot, uint32_t op_id, uint8_t* out_buf, uint32_t out_cap, uint32_t* out_len)
{
    const num8lora_sender_op_t* op = find_op(s, op_id);
    num8lora_op_data_payload_t p;

    if (op == NULL)
    {
        return 0;
    }

    p.stream_id = s->stream_id;
    p.op_id = op->op_id;
    p.op_type = op->op_type;
    p.reserved0 = 0u;
    p.reserved1 = 0u;
    p.value = op->value;

    if (!num8lora_op_encode_data(out_buf, out_cap, s->sender_id, slot->receiver_id, s->seq++, &p, out_len))
    {
        return 0;
    }

    slot->inflight_op_id = op_id;
    slot->waiting_ack = 1u;
    return 1;
}

void num8lora_sender_init(
    num8lora_sender_t* s,
    uint16_t sender_id,
    uint32_t stream_id,
    uint32_t ack_timeout_ms,
    uint8_t max_retries,
    num8lora_sender_op_t* ops_buf,
    uint32_t ops_capacity,
    num8lora_sender_receiver_slot_t* receiver_buf,
    uint32_t receiver_capacity)
{
    if (s == NULL)
    {
        return;
    }

    memset(s, 0, sizeof(*s));
    s->sender_id = sender_id;
    s->seq = 1u;
    s->stream_id = stream_id;
    s->ack_timeout_ms = ack_timeout_ms;
    s->max_retries = max_retries;
    s->ops = ops_buf;
    s->op_capacity = ops_capacity;
    s->receivers = receiver_buf;
    s->receiver_capacity = receiver_capacity;

    if (receiver_buf != NULL && receiver_capacity > 0u)
    {
        memset(receiver_buf, 0, sizeof(*receiver_buf) * receiver_capacity);
    }
}

int num8lora_sender_register_receiver(num8lora_sender_t* s, uint16_t receiver_id, uint32_t last_acked_op_id)
{
    uint32_t i;

    if (s == NULL || s->receivers == NULL || receiver_id == 0u)
    {
        return 0;
    }
    if (find_slot(s, receiver_id) != NULL)
    {
        return 1;
    }

    for (i = 0; i < s->receiver_capacity; ++i)
    {
        if (!s->receivers[i].active)
        {
            s->receivers[i].active = 1u;
            s->receivers[i].receiver_id = receiver_id;
            s->receivers[i].last_acked_op_id = last_acked_op_id;
            return 1;
        }
    }
    return 0;
}

int num8lora_sender_set_receiver_progress(num8lora_sender_t* s, uint16_t receiver_id, uint32_t last_acked_op_id)
{
    num8lora_sender_receiver_slot_t* slot;
    if (s == NULL)
    {
        return 0;
    }
    slot = find_slot(s, receiver_id);
    if (slot == NULL)
    {
        return 0;
    }
    slot->last_acked_op_id = last_acked_op_id;
    slot->waiting_ack = 0u;
    slot->inflight_op_id = 0u;
    slot->retries = 0u;
    return 1;
}

static int enqueue_op(num8lora_sender_t* s, uint8_t type, uint32_t value, uint32_t* out_op_id)
{
    num8lora_sender_op_t* op;
    if (s == NULL || s->ops == NULL || type == 0u || value > MAX_VALUE)
    {
        return 0;
    }
    if (s->op_count >= s->op_capacity)
    {
        return 0;
    }

    s->latest_op_id++;
    op = &s->ops[s->op_count++];
    op->op_id = s->latest_op_id;
    op->op_type = type;
    op->value = value;
    if (out_op_id != NULL)
    {
        *out_op_id = op->op_id;
    }
    return 1;
}

int num8lora_sender_enqueue_add(num8lora_sender_t* s, uint32_t value, uint32_t* out_op_id)
{
    return enqueue_op(s, NUM8LORA_OP_ADD, value, out_op_id);
}

int num8lora_sender_enqueue_remove(num8lora_sender_t* s, uint32_t value, uint32_t* out_op_id)
{
    return enqueue_op(s, NUM8LORA_OP_REMOVE, value, out_op_id);
}

int num8lora_sender_enqueue_lists(
    num8lora_sender_t* s,
    const uint32_t* remove_values,
    uint32_t remove_count,
    const uint32_t* add_values,
    uint32_t add_count,
    uint32_t* out_first_op_id,
    uint32_t* out_last_op_id)
{
    uint32_t i;
    uint32_t first = 0u;
    uint32_t last = 0u;

    if (s == NULL)
    {
        return 0;
    }

    for (i = 0; i < remove_count; ++i)
    {
        if (!num8lora_sender_enqueue_remove(s, remove_values[i], &last))
        {
            return 0;
        }
        if (first == 0u)
        {
            first = last;
        }
    }

    for (i = 0; i < add_count; ++i)
    {
        if (!num8lora_sender_enqueue_add(s, add_values[i], &last))
        {
            return 0;
        }
        if (first == 0u)
        {
            first = last;
        }
    }

    if (out_first_op_id != NULL)
    {
        *out_first_op_id = first;
    }
    if (out_last_op_id != NULL)
    {
        *out_last_op_id = last;
    }
    return 1;
}

int num8lora_sender_build_beacon(const num8lora_sender_t* s, uint8_t* out_buf, uint32_t out_cap, uint32_t* out_len)
{
    num8lora_op_beacon_payload_t p;
    if (s == NULL)
    {
        return 0;
    }
    p.stream_id = s->stream_id;
    p.latest_op_id = s->latest_op_id;
    return num8lora_op_encode_beacon(out_buf, out_cap, s->sender_id, s->seq, &p, out_len);
}

int num8lora_sender_poll_tx(
    num8lora_sender_t* s,
    uint64_t now_ms,
    uint8_t* out_buf,
    uint32_t out_cap,
    uint32_t* out_len,
    uint16_t* out_target_receiver_id)
{
    uint32_t i;

    if (s == NULL || out_buf == NULL || out_len == NULL || out_target_receiver_id == NULL)
    {
        return 0;
    }

    for (i = 0; i < s->receiver_capacity; ++i)
    {
        num8lora_sender_receiver_slot_t* slot = &s->receivers[i];
        uint32_t target;

        if (!slot->active)
        {
            continue;
        }

        if (slot->waiting_ack)
        {
            if (now_ms < slot->due_at_ms)
            {
                continue;
            }
            if (slot->retries >= s->max_retries)
            {
                slot->waiting_ack = 0u;
                slot->inflight_op_id = 0u;
                slot->retries = 0u;
                continue;
            }
            slot->retries++;
            if (!emit_data(s, slot, slot->inflight_op_id, out_buf, out_cap, out_len))
            {
                return 0;
            }
            slot->due_at_ms = now_ms + s->ack_timeout_ms;
            *out_target_receiver_id = slot->receiver_id;
            return 1;
        }

        target = slot->last_acked_op_id + 1u;
        if (target == 0u || target > s->latest_op_id)
        {
            continue;
        }

        if (!emit_data(s, slot, target, out_buf, out_cap, out_len))
        {
            return 0;
        }
        slot->retries = 0u;
        slot->due_at_ms = now_ms + s->ack_timeout_ms;
        *out_target_receiver_id = slot->receiver_id;
        return 1;
    }

    return 0;
}

int num8lora_sender_handle_rx(
    num8lora_sender_t* s,
    uint64_t now_ms,
    const uint8_t* in_buf,
    uint32_t in_len)
{
    num8lora_op_common_header_t hdr;

    if (s == NULL || in_buf == NULL)
    {
        return 0;
    }
    if (!num8lora_op_decode_common_header(in_buf, in_len, &hdr))
    {
        return 0;
    }

    if (hdr.msg_type == NUM8LORA_OP_MSG_REQUEST)
    {
        num8lora_op_request_payload_t p;
        num8lora_sender_receiver_slot_t* slot;
        if (!num8lora_op_decode_request(in_buf, in_len, &hdr, &p))
        {
            return 0;
        }
        if (p.stream_id != s->stream_id)
        {
            return 0;
        }
        if (!num8lora_sender_register_receiver(s, hdr.sender_id, p.next_needed_op_id == 0u ? 0u : p.next_needed_op_id - 1u))
        {
            return 0;
        }
        slot = find_slot(s, hdr.sender_id);
        if (slot == NULL)
        {
            return 0;
        }
        slot->last_acked_op_id = p.next_needed_op_id == 0u ? 0u : p.next_needed_op_id - 1u;
        slot->waiting_ack = 0u;
        slot->due_at_ms = now_ms;
        return 1;
    }

    if (hdr.msg_type == NUM8LORA_OP_MSG_ACK)
    {
        num8lora_op_ack_payload_t p;
        num8lora_sender_receiver_slot_t* slot;
        if (!num8lora_op_decode_ack(in_buf, in_len, &hdr, &p))
        {
            return 0;
        }
        if (p.stream_id != s->stream_id)
        {
            return 0;
        }
        slot = find_slot(s, hdr.sender_id);
        if (slot == NULL)
        {
            return 0;
        }
        if (p.ack_op_id > slot->last_acked_op_id)
        {
            slot->last_acked_op_id = p.ack_op_id;
        }
        slot->waiting_ack = 0u;
        slot->inflight_op_id = 0u;
        slot->retries = 0u;
        return 1;
    }

    if (hdr.msg_type == NUM8LORA_OP_MSG_NACK)
    {
        num8lora_op_nack_payload_t p;
        num8lora_sender_receiver_slot_t* slot;
        if (!num8lora_op_decode_nack(in_buf, in_len, &hdr, &p))
        {
            return 0;
        }
        if (p.stream_id != s->stream_id)
        {
            return 0;
        }
        slot = find_slot(s, hdr.sender_id);
        if (slot == NULL)
        {
            return 0;
        }
        if (p.expected_next_op_id > 0u)
        {
            slot->last_acked_op_id = p.expected_next_op_id - 1u;
        }
        slot->waiting_ack = 0u;
        slot->inflight_op_id = 0u;
        slot->retries = 0u;
        return 1;
    }

    return 0;
}
