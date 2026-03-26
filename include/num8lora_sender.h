#ifndef NUM8LORA_SENDER_H
#define NUM8LORA_SENDER_H

#include <stdint.h>
#include "num8lora_op.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct num8lora_sender_op_s
{
    uint32_t op_id;
    uint8_t op_type;
    uint32_t value;
} num8lora_sender_op_t;

typedef struct num8lora_sender_receiver_slot_s
{
    uint16_t receiver_id;
    uint32_t last_acked_op_id;
    uint32_t inflight_op_id;
    uint64_t due_at_ms;
    uint8_t waiting_ack;
    uint8_t retries;
    uint8_t active;
} num8lora_sender_receiver_slot_t;

typedef struct num8lora_sender_s
{
    uint16_t sender_id;
    uint16_t seq;
    uint32_t stream_id;
    uint32_t latest_op_id;

    uint32_t ack_timeout_ms;
    uint8_t max_retries;

    num8lora_sender_op_t* ops;
    uint32_t op_capacity;
    uint32_t op_count;

    num8lora_sender_receiver_slot_t* receivers;
    uint32_t receiver_capacity;
} num8lora_sender_t;

NUM8LORA_OP_API void num8lora_sender_init(
    num8lora_sender_t* s,
    uint16_t sender_id,
    uint32_t stream_id,
    uint32_t ack_timeout_ms,
    uint8_t max_retries,
    num8lora_sender_op_t* ops_buf,
    uint32_t ops_capacity,
    num8lora_sender_receiver_slot_t* receiver_buf,
    uint32_t receiver_capacity);

NUM8LORA_OP_API int num8lora_sender_register_receiver(num8lora_sender_t* s, uint16_t receiver_id, uint32_t last_acked_op_id);
NUM8LORA_OP_API int num8lora_sender_set_receiver_progress(num8lora_sender_t* s, uint16_t receiver_id, uint32_t last_acked_op_id);

NUM8LORA_OP_API int num8lora_sender_enqueue_add(num8lora_sender_t* s, uint32_t value, uint32_t* out_op_id);
NUM8LORA_OP_API int num8lora_sender_enqueue_remove(num8lora_sender_t* s, uint32_t value, uint32_t* out_op_id);
NUM8LORA_OP_API int num8lora_sender_enqueue_lists(
    num8lora_sender_t* s,
    const uint32_t* remove_values,
    uint32_t remove_count,
    const uint32_t* add_values,
    uint32_t add_count,
    uint32_t* out_first_op_id,
    uint32_t* out_last_op_id);

NUM8LORA_OP_API int num8lora_sender_build_beacon(const num8lora_sender_t* s, uint8_t* out_buf, uint32_t out_cap, uint32_t* out_len);

NUM8LORA_OP_API int num8lora_sender_poll_tx(
    num8lora_sender_t* s,
    uint64_t now_ms,
    uint8_t* out_buf,
    uint32_t out_cap,
    uint32_t* out_len,
    uint16_t* out_target_receiver_id);

NUM8LORA_OP_API int num8lora_sender_handle_rx(
    num8lora_sender_t* s,
    uint64_t now_ms,
    const uint8_t* in_buf,
    uint32_t in_len);

#ifdef __cplusplus
}
#endif

#endif
