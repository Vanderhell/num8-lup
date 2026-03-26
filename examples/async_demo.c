#include "num8lora_sender.h"
#include "num8lora_receiver.h"

#include <stdio.h>

typedef struct demo_state_s { int bits[1000]; } demo_state_t;

static int apply_mem(void* u, uint8_t op_type, uint32_t value)
{
    demo_state_t* s = (demo_state_t*)u;
    if (value >= 1000u) return 0;
    if (op_type == NUM8LORA_OP_ADD) s->bits[value] = 1;
    else if (op_type == NUM8LORA_OP_REMOVE) s->bits[value] = 0;
    else return 0;
    return 1;
}

int main(void)
{
    num8lora_sender_t sender;
    num8lora_sender_op_t op_store[128];
    num8lora_sender_receiver_slot_t rx_slots[16];

    num8lora_receiver_t r1, r2;
    demo_state_t s1 = {0}, s2 = {0};

    uint8_t beacon[64], req[64], tx[128], resp[128];
    uint32_t beacon_len = 0, req_len = 0, tx_len = 0, resp_len = 0;
    uint16_t target = 0;
    uint64_t now = 0;
    int loops = 0;

    num8lora_sender_init(&sender, 10u, 5001u, 100u, 3u, op_store, 128u, rx_slots, 16u);
    num8lora_sender_register_receiver(&sender, 21u, 0u);
    num8lora_sender_register_receiver(&sender, 22u, 0u);

    num8lora_receiver_init(&r1, 21u, 10u, 5001u, 0u);
    num8lora_receiver_init(&r2, 22u, 10u, 5001u, 0u);

    num8lora_sender_enqueue_remove(&sender, 5u, NULL);
    num8lora_sender_enqueue_add(&sender, 100u, NULL);
    num8lora_sender_enqueue_add(&sender, 200u, NULL);

    num8lora_sender_build_beacon(&sender, beacon, sizeof(beacon), &beacon_len);

    if (num8lora_receiver_handle_beacon(&r1, beacon, beacon_len, req, sizeof(req), &req_len))
        num8lora_sender_handle_rx(&sender, now, req, req_len);
    if (num8lora_receiver_handle_beacon(&r2, beacon, beacon_len, req, sizeof(req), &req_len))
        num8lora_sender_handle_rx(&sender, now, req, req_len);

    while (loops++ < 200)
    {
        if (!num8lora_sender_poll_tx(&sender, now, tx, sizeof(tx), &tx_len, &target))
        {
            now += 10;
            continue;
        }

        if (target == 21u)
        {
            if (num8lora_receiver_handle_data(&r1, tx, tx_len, apply_mem, &s1, resp, sizeof(resp), &resp_len))
                num8lora_sender_handle_rx(&sender, now, resp, resp_len);
        }
        else if (target == 22u)
        {
            if (num8lora_receiver_handle_data(&r2, tx, tx_len, apply_mem, &s2, resp, sizeof(resp), &resp_len))
                num8lora_sender_handle_rx(&sender, now, resp, resp_len);
        }

        now += 5;

        if (r1.last_applied_op_id == sender.latest_op_id && r2.last_applied_op_id == sender.latest_op_id)
            break;
    }

    printf("demo complete, latest_op=%u r1=%u r2=%u\n",
        (unsigned)sender.latest_op_id,
        (unsigned)r1.last_applied_op_id,
        (unsigned)r2.last_applied_op_id);

    printf("r1: value100=%d value200=%d\n", s1.bits[100], s1.bits[200]);
    printf("r2: value100=%d value200=%d\n", s2.bits[100], s2.bits[200]);

    return 0;
}
