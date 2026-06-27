#include "num8lora_sender.h"

#include <string.h>
#include <limits.h>

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
    uint32_t offset;
    uint32_t index;

    if (op_id < s->oldest_retained_op_id || op_id > s->latest_op_id || s->op_count == 0u)
    {
        return NULL;
    }
    offset = op_id - s->oldest_retained_op_id;
    if (offset >= s->op_count || s->op_capacity == 0u)
    {
        return NULL;
    }
    index = s->op_head + offset;
    if (index >= s->op_capacity)
    {
        index %= s->op_capacity;
    }
    return &s->ops[index];
}

static uint64_t deadline_after(uint64_t now_ms, uint32_t timeout_ms)
{
    if (timeout_ms == 0u)
    {
        return now_ms;
    }
    if (UINT64_MAX - now_ms < (uint64_t)timeout_ms)
    {
        return UINT64_MAX;
    }
    return now_ms + (uint64_t)timeout_ms;
}

static uint32_t sender_active_floor(const num8lora_sender_t* s)
{
    uint32_t floor = s->latest_op_id + 1u;
    uint32_t i;
    uint8_t any_active = 0u;

    for (i = 0; i < s->receiver_capacity; ++i)
    {
        const num8lora_sender_receiver_slot_t* slot = &s->receivers[i];
        uint32_t needed;

        if (!slot->active)
        {
            continue;
        }

        any_active = 1u;
        needed = slot->last_acked_op_id + 1u;
        if (needed < floor)
        {
            floor = needed;
        }
    }

    if (!any_active)
    {
        floor = s->latest_op_id + 1u;
    }

    return floor;
}

static void sender_reclaim_history(num8lora_sender_t* s)
{
    uint32_t floor;
    uint32_t drop_count;

    if (s == NULL)
    {
        return;
    }

    floor = sender_active_floor(s);
    if (floor <= s->oldest_retained_op_id)
    {
        return;
    }

    if (floor > s->latest_op_id + 1u)
    {
        floor = s->latest_op_id + 1u;
    }

    drop_count = floor - s->oldest_retained_op_id;
    if (drop_count > s->op_count)
    {
        drop_count = s->op_count;
    }

    s->oldest_retained_op_id = floor;
    s->op_head = (s->op_head + drop_count) % (s->op_capacity == 0u ? 1u : s->op_capacity);
    s->op_count -= drop_count;

    if (s->op_count == 0u)
    {
        s->op_head = 0u;
    }
}

static int request_is_within_sender_history(const num8lora_sender_t* s, const num8lora_sender_receiver_slot_t* slot, uint32_t next_needed_op_id)
{
    if (next_needed_op_id == 0u)
    {
        return 0;
    }
    if (slot == NULL || !slot->active)
    {
        return 0;
    }
    if (next_needed_op_id < s->oldest_retained_op_id)
    {
        return 0;
    }
    if (next_needed_op_id > s->latest_op_id + 1u)
    {
        return 0;
    }
    if (next_needed_op_id != slot->last_acked_op_id + 1u)
    {
        return 0;
    }
    return 1;
}

static int emit_data(num8lora_sender_t* s, num8lora_sender_receiver_slot_t* slot, uint32_t op_id, uint8_t* out_buf, uint32_t out_cap, uint32_t* out_len, uint64_t now_ms)
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

    if (s->seq == UINT16_MAX)
    {
        return 0;
    }

    if (!num8lora_op_encode_data(out_buf, out_cap, s->sender_id, slot->receiver_id, s->seq++, &p, out_len))
    {
        return 0;
    }

    slot->inflight_op_id = op_id;
    slot->waiting_ack = 1u;
    slot->exhausted = 0u;
    slot->due_at_ms = deadline_after(now_ms, s->ack_timeout_ms);
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
    s->oldest_retained_op_id = 1u;
    s->op_head = 0u;
    s->poll_cursor = 0u;
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
    sender_reclaim_history(s);
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
            s->receivers[i].exhausted = 0u;
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
    slot->exhausted = 0u;
    sender_reclaim_history(s);
    return 1;
}

int num8lora_sender_unregister_receiver(num8lora_sender_t* s, uint16_t receiver_id)
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

    slot->active = 0u;
    slot->waiting_ack = 0u;
    slot->inflight_op_id = 0u;
    slot->retries = 0u;
    slot->exhausted = 0u;
    sender_reclaim_history(s);
    return 1;
}

static int enqueue_op(num8lora_sender_t* s, uint8_t type, uint32_t value, uint32_t* out_op_id)
{
    num8lora_sender_op_t* op;
    uint32_t slot_index;

    if (s == NULL || s->ops == NULL || type == 0u || value > MAX_VALUE)
    {
        return 0;
    }
    sender_reclaim_history(s);
    if (s->op_capacity == 0u || s->op_count >= s->op_capacity)
    {
        return 0;
    }

    s->latest_op_id++;
    if (s->op_count == 0u)
    {
        s->op_head = 0u;
        s->oldest_retained_op_id = s->latest_op_id;
    }

    slot_index = s->op_head + s->op_count;
    if (slot_index >= s->op_capacity)
    {
        slot_index %= s->op_capacity;
    }
    op = &s->ops[slot_index];
    op->op_id = s->latest_op_id;
    op->op_type = type;
    op->value = value;
    s->op_count++;
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
    if (s->seq == UINT16_MAX)
    {
        return 0;
    }
    return num8lora_op_encode_beacon(out_buf, out_cap, s->sender_id, s->seq, &p, out_len);
}

int num8lora_sender_handle_request(
    num8lora_sender_t* s,
    const uint8_t* in_buf,
    uint32_t in_len,
    uint8_t* out_response_buf,
    uint32_t out_cap,
    uint32_t* out_len)
{
    num8lora_op_common_header_t hdr = {0};
    num8lora_op_request_payload_t p = {0};
    num8lora_sender_receiver_slot_t* slot;
    num8lora_op_nack_payload_t nack = {0};

    if (out_len != NULL)
    {
        *out_len = 0u;
    }
    if (s == NULL || in_buf == NULL || out_response_buf == NULL || out_len == NULL || in_len < 2u)
    {
        return 0;
    }

    if (!num8lora_op_decode_request(in_buf, in_len, &hdr, &p))
    {
        return 0;
    }

    slot = find_slot(s, hdr.sender_id);
    if (slot == NULL || !slot->active || hdr.protocol_version != NUM8LORA_OP_PROTOCOL_VERSION || hdr.receiver_id != s->sender_id)
    {
        return 0;
    }
    if (p.stream_id != s->stream_id)
    {
        return 0;
    }
    if (p.next_needed_op_id < s->oldest_retained_op_id)
    {
        nack.stream_id = s->stream_id;
        nack.error_code = NUM8LORA_OP_ERR_HISTORY_UNAVAILABLE;
        nack.detail = 0u;
        nack.expected_next_op_id = s->oldest_retained_op_id;
        if (s->seq == UINT16_MAX)
        {
            return 0;
        }
        return num8lora_op_encode_nack(out_response_buf, out_cap, s->sender_id, slot->receiver_id, s->seq++, &nack, out_len);
    }
    return 0;
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
    uint32_t start;

    if (out_len != NULL)
    {
        *out_len = 0u;
    }
    if (out_target_receiver_id != NULL)
    {
        *out_target_receiver_id = 0u;
    }
    if (s == NULL || out_buf == NULL || out_len == NULL || out_target_receiver_id == NULL)
    {
        return 0;
    }

    sender_reclaim_history(s);
    if (s->receiver_capacity == 0u)
    {
        return 0;
    }

    start = s->poll_cursor % s->receiver_capacity;
    for (i = 0; i < s->receiver_capacity; ++i)
    {
        uint32_t index = start + i;
        num8lora_sender_receiver_slot_t* slot;
        uint32_t target;

        if (index >= s->receiver_capacity)
        {
            index %= s->receiver_capacity;
        }
        slot = &s->receivers[index];

        if (!slot->active || slot->exhausted)
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
                slot->exhausted = 1u;
                continue;
            }
            slot->retries++;
            if (!emit_data(s, slot, slot->inflight_op_id, out_buf, out_cap, out_len, now_ms))
            {
                return 0;
            }
            *out_target_receiver_id = slot->receiver_id;
            s->poll_cursor = (index + 1u) % s->receiver_capacity;
            return 1;
        }

        target = slot->last_acked_op_id + 1u;
        if (target == 0u || target > s->latest_op_id || target < s->oldest_retained_op_id)
        {
            continue;
        }

        if (!emit_data(s, slot, target, out_buf, out_cap, out_len, now_ms))
        {
            return 0;
        }
        slot->retries = 0u;
        *out_target_receiver_id = slot->receiver_id;
        s->poll_cursor = (index + 1u) % s->receiver_capacity;
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
    num8lora_op_request_payload_t p;
    num8lora_sender_receiver_slot_t* slot;

    if (s == NULL || in_buf == NULL || in_len < 2u)
    {
        return 0;
    }

    if (in_buf[1] == NUM8LORA_OP_MSG_REQUEST)
    {
        num8lora_op_request_payload_t req = {0};
        if (!num8lora_op_decode_request(in_buf, in_len, &hdr, &req))
        {
            return 0;
        }
        slot = find_slot(s, hdr.sender_id);
        if (slot == NULL || !slot->active || hdr.protocol_version != NUM8LORA_OP_PROTOCOL_VERSION || hdr.receiver_id != s->sender_id)
        {
            return 0;
        }
        if (req.stream_id != s->stream_id)
        {
            return 0;
        }
        if (req.next_needed_op_id < s->oldest_retained_op_id)
        {
            return 0;
        }
        if (req.next_needed_op_id > s->latest_op_id + 1u)
        {
            return 0;
        }
        if (slot->exhausted && req.next_needed_op_id == slot->last_acked_op_id + 1u)
        {
            return 0;
        }
        if (req.next_needed_op_id != slot->last_acked_op_id + 1u)
        {
            return 0;
        }
        if (slot->waiting_ack && slot->inflight_op_id != req.next_needed_op_id)
        {
            return 0;
        }
        slot->due_at_ms = now_ms;
        return 1;
    }

    if (in_buf[1] == NUM8LORA_OP_MSG_ACK)
    {
        num8lora_op_ack_payload_t ack = {0};
        if (!num8lora_op_decode_ack(in_buf, in_len, &hdr, &ack))
        {
            return 0;
        }
        slot = find_slot(s, hdr.sender_id);
        if (slot == NULL || !slot->active || hdr.protocol_version != NUM8LORA_OP_PROTOCOL_VERSION || hdr.receiver_id != s->sender_id)
        {
            return 0;
        }
        if (ack.stream_id != s->stream_id || !slot->waiting_ack || slot->inflight_op_id == 0u || ack.ack_op_id != slot->inflight_op_id)
        {
            return 0;
        }
        slot->last_acked_op_id = ack.ack_op_id;
        slot->waiting_ack = 0u;
        slot->inflight_op_id = 0u;
        slot->retries = 0u;
        slot->exhausted = 0u;
        sender_reclaim_history(s);
        return 1;
    }

    if (in_buf[1] == NUM8LORA_OP_MSG_NACK)
    {
        num8lora_op_nack_payload_t nack = {0};
        if (!num8lora_op_decode_nack(in_buf, in_len, &hdr, &nack))
        {
            return 0;
        }
        slot = find_slot(s, hdr.sender_id);
        if (slot == NULL || !slot->active || hdr.protocol_version != NUM8LORA_OP_PROTOCOL_VERSION || hdr.receiver_id != s->sender_id)
        {
            return 0;
        }
        if (nack.stream_id != s->stream_id || !slot->waiting_ack || slot->inflight_op_id == 0u)
        {
            return 0;
        }
        if (nack.expected_next_op_id < s->oldest_retained_op_id || nack.expected_next_op_id > s->latest_op_id + 1u)
        {
            return 0;
        }
        if (nack.expected_next_op_id != slot->inflight_op_id)
        {
            return 0;
        }
        slot->waiting_ack = 0u;
        slot->inflight_op_id = 0u;
        slot->retries = 0u;
        slot->exhausted = 0u;
        slot->due_at_ms = now_ms;
        sender_reclaim_history(s);
        return 1;
    }

    return 0;
}
