#ifndef NUM8LORA_RECEIVER_H
#define NUM8LORA_RECEIVER_H

#include <stdint.h>
#include "num8lora_op.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*num8lora_receiver_apply_fn)(void* user, uint8_t op_type, uint32_t value);

typedef struct num8lora_receiver_s
{
    uint16_t receiver_id;
    uint16_t expected_sender_id;
    uint16_t seq;

    uint32_t stream_id;
    uint32_t last_applied_op_id;
} num8lora_receiver_t;

void num8lora_receiver_init(num8lora_receiver_t* r, uint16_t receiver_id, uint16_t expected_sender_id, uint32_t stream_id, uint32_t last_applied_op_id);

int num8lora_receiver_handle_beacon(
    num8lora_receiver_t* r,
    const uint8_t* in_buf,
    uint32_t in_len,
    uint8_t* out_buf,
    uint32_t out_cap,
    uint32_t* out_len);

int num8lora_receiver_handle_data(
    num8lora_receiver_t* r,
    const uint8_t* in_buf,
    uint32_t in_len,
    num8lora_receiver_apply_fn apply_fn,
    void* apply_user,
    uint8_t* out_buf,
    uint32_t out_cap,
    uint32_t* out_len);

#ifdef NUM8LORA_ENABLE_NUM8
#include "num8.h"
int num8lora_receiver_apply_num8(void* user, uint8_t op_type, uint32_t value);
#endif

#ifdef __cplusplus
}
#endif

#endif
