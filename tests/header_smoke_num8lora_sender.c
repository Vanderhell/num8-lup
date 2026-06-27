#include "num8lora_sender.h"

int main(void)
{
    num8lora_sender_t ctx;
    num8lora_sender_op_t ops[1];
    num8lora_sender_receiver_slot_t slots[1];

    num8lora_sender_init(&ctx, 1u, 1u, 0u, 1u, ops, 1u, slots, 1u);
    return 0;
}
