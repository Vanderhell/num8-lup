#include "num8lora_sender.h"
#include "num8lora_receiver.h"

#include <stdio.h>

typedef struct consumer_state_s
{
    int present[100];
} consumer_state_t;

static int apply_fn(void* user, uint8_t op_type, uint32_t value)
{
    consumer_state_t* s = (consumer_state_t*)user;

    if (value >= 100u)
    {
        return 0;
    }
    if (op_type == NUM8LORA_OP_ADD)
    {
        s->present[value] = 1;
        return 1;
    }
    if (op_type == NUM8LORA_OP_REMOVE)
    {
        s->present[value] = 0;
        return 1;
    }
    return 0;
}

int main(void)
{
    num8lora_sender_t sender;
    num8lora_sender_op_t ops[2];
    num8lora_sender_receiver_slot_t slots[1];
    num8lora_receiver_t receiver;
    consumer_state_t state = { 0 };
    uint8_t out[128];
    uint32_t len = 0u;
    uint16_t target = 0u;
    num8lora_op_data_payload_t data = { 42u, 1u, NUM8LORA_OP_ADD, 0u, 0u, 7u };

    num8lora_sender_init(&sender, 9u, 100u, 10u, 1u, ops, 2u, slots, 1u);
    num8lora_sender_register_receiver(&sender, 10u, 0u);
    if (!num8lora_sender_enqueue_add(&sender, 7u, NULL))
    {
        return 1;
    }
    if (!num8lora_sender_poll_tx(&sender, 0u, out, sizeof(out), &len, &target))
    {
        return 2;
    }

    num8lora_receiver_init(&receiver, 10u, 9u, 100u, 0u);
    if (!num8lora_op_encode_data(out, sizeof(out), 9u, 10u, 1u, &data, &len))
    {
        return 3;
    }
    if (!num8lora_receiver_handle_data(&receiver, out, len, apply_fn, &state, out, sizeof(out), &len))
    {
        return 4;
    }
    puts("async consumer ok");
    return 0;
}
