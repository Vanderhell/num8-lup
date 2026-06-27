#include "num8lora_sender.h"
#include "num8lora_receiver.h"

#include <stdio.h>

#define C(x) do { if(!(x)) { fprintf(stderr,"FAIL %d\n",__LINE__); return 1; } } while(0)

typedef struct state_s { int present[1000]; } state_t;

static int apply_fn(void* u, uint8_t op_type, uint32_t value)
{
    state_t* s=(state_t*)u;
    if(value>=1000u) return 0;
    if(op_type==NUM8LORA_OP_ADD) s->present[value]=1;
    else if(op_type==NUM8LORA_OP_REMOVE) s->present[value]=0;
    else return 0;
    return 1;
}

static int test_sender_routing_gates(void)
{
    num8lora_sender_t snd;
    num8lora_sender_op_t ops[8];
    num8lora_sender_receiver_slot_t slots[4];
    uint8_t out[128];
    uint32_t out_len = 0;
    uint16_t target = 0;
    uint32_t sent_len = 0;
    uint32_t now = 0;
    num8lora_op_request_payload_t req;

    num8lora_sender_init(&snd, 10u, 1001u, 50u, 3u, ops, 8u, slots, 4u);
    C(num8lora_sender_register_receiver(&snd, 21u, 0u));
    C(num8lora_sender_register_receiver(&snd, 22u, 0u));
    C(num8lora_sender_enqueue_add(&snd, 5u, NULL));
    C(num8lora_sender_enqueue_add(&snd, 6u, NULL));

    C(num8lora_sender_poll_tx(&snd, now, out, sizeof(out), &sent_len, &target));
    C(target == 21u);
    C(slots[0].waiting_ack == 1u && slots[0].inflight_op_id == 1u);

    req.stream_id = snd.stream_id;
    req.next_needed_op_id = 1u;
    C(num8lora_op_encode_request(out, sizeof(out), 21u, 99u, 1u, &req, &out_len));
    C(!num8lora_sender_handle_rx(&snd, now, out, out_len));
    C(slots[0].waiting_ack == 1u && slots[0].inflight_op_id == 1u && slots[0].last_acked_op_id == 0u);

    C(num8lora_op_encode_request(out, sizeof(out), 0u, 10u, 1u, &req, &out_len));
    C(!num8lora_sender_handle_rx(&snd, now, out, out_len));
    C(slots[0].waiting_ack == 1u && slots[0].inflight_op_id == 1u);

    C(num8lora_op_encode_ack(out, sizeof(out), 77u, 10u, 1u, &(num8lora_op_ack_payload_t){ snd.stream_id, 1u }, &out_len));
    C(!num8lora_sender_handle_rx(&snd, now, out, out_len));
    C(slots[0].waiting_ack == 1u && slots[0].inflight_op_id == 1u);

    C(num8lora_op_encode_nack(out, sizeof(out), 77u, 10u, 1u, &(num8lora_op_nack_payload_t){ snd.stream_id, NUM8LORA_OP_ERR_STREAM, 0u, 1u }, &out_len));
    C(!num8lora_sender_handle_rx(&snd, now, out, out_len));
    C(slots[0].waiting_ack == 1u && slots[0].inflight_op_id == 1u);

    req.stream_id = snd.stream_id;
    req.next_needed_op_id = 99u;
    C(num8lora_op_encode_request(out, sizeof(out), 21u, 10u, 2u, &req, &out_len));
    C(!num8lora_sender_handle_rx(&snd, now, out, out_len));
    C(slots[0].waiting_ack == 1u && slots[0].inflight_op_id == 1u);

    C(num8lora_op_encode_ack(out, sizeof(out), 21u, 10u, 2u, &(num8lora_op_ack_payload_t){ snd.stream_id, 99u }, &out_len));
    C(!num8lora_sender_handle_rx(&snd, now, out, out_len));
    C(slots[0].waiting_ack == 1u && slots[0].inflight_op_id == 1u);

    C(num8lora_op_encode_nack(out, sizeof(out), 21u, 10u, 2u, &(num8lora_op_nack_payload_t){ snd.stream_id, NUM8LORA_OP_ERR_SEQUENCE_GAP, 0u, 99u }, &out_len));
    C(!num8lora_sender_handle_rx(&snd, now, out, out_len));
    C(slots[0].waiting_ack == 1u && slots[0].inflight_op_id == 1u);

    req.stream_id = snd.stream_id;
    req.next_needed_op_id = 1u;
    C(num8lora_op_encode_request(out, sizeof(out), 21u, 10u, 3u, &req, &out_len));
    C(num8lora_sender_handle_rx(&snd, now, out, out_len));
    C(slots[0].waiting_ack == 1u && slots[0].inflight_op_id == 1u);

    C(num8lora_op_encode_ack(out, sizeof(out), 21u, 10u, 4u, &(num8lora_op_ack_payload_t){ snd.stream_id, 1u }, &out_len));
    C(num8lora_sender_handle_rx(&snd, now, out, out_len));
    C(slots[0].waiting_ack == 0u && slots[0].inflight_op_id == 0u && slots[0].last_acked_op_id == 1u);

    C(num8lora_sender_poll_tx(&snd, now, out, sizeof(out), &sent_len, &target));
    C(target == 21u);
    C(slots[0].waiting_ack == 1u && slots[0].inflight_op_id == 2u);

    C(num8lora_op_encode_ack(out, sizeof(out), 21u, 10u, 5u, &(num8lora_op_ack_payload_t){ snd.stream_id, 1u }, &out_len));
    C(!num8lora_sender_handle_rx(&snd, now, out, out_len));
    C(slots[0].waiting_ack == 1u && slots[0].inflight_op_id == 2u && slots[0].last_acked_op_id == 1u);

    C(num8lora_op_encode_ack(out, sizeof(out), 21u, 10u, 6u, &(num8lora_op_ack_payload_t){ snd.stream_id, 2u }, &out_len));
    C(num8lora_sender_handle_rx(&snd, now, out, out_len));
    C(slots[0].waiting_ack == 0u && slots[0].inflight_op_id == 0u && slots[0].last_acked_op_id == 2u);

    return 0;
}

static int test_receiver_routing_gates(void)
{
    num8lora_receiver_t rx;
    state_t s = {0};
    uint8_t out[128];
    uint32_t out_len = 777u;
    num8lora_op_data_payload_t d;

    num8lora_receiver_init(&rx, 21u, 10u, 1001u, 0u);

    d.stream_id = rx.stream_id;
    d.op_id = 1u;
    d.op_type = NUM8LORA_OP_ADD;
    d.reserved0 = 0u;
    d.reserved1 = 0u;
    d.value = 5u;

    C(num8lora_op_encode_data(out, sizeof(out), 10u, 22u, 1u, &d, &out_len));
    out_len = 777u;
    C(!num8lora_receiver_handle_data(&rx, out, out_len, apply_fn, &s, out, sizeof(out), &out_len));
    C(out_len == 0u);
    C(rx.last_applied_op_id == 0u && s.present[5] == 0);

    C(num8lora_op_encode_data(out, sizeof(out), 10u, 21u, 1u, &d, &out_len));
    out_len = 777u;
    d.stream_id = 9999u;
    C(!num8lora_receiver_handle_data(&rx, out, out_len, apply_fn, &s, out, sizeof(out), &out_len));
    C(out_len == 0u);
    C(rx.last_applied_op_id == 0u && s.present[5] == 0);

    out_len = 123u;
    C(!num8lora_receiver_handle_data(&rx, out, 0u, apply_fn, &s, out, sizeof(out), &out_len));
    C(out_len == 0u);

    return 0;
}

int main(void)
{
    C(test_sender_routing_gates() == 0);
    C(test_receiver_routing_gates() == 0);

    num8lora_sender_t snd;
    num8lora_sender_op_t ops[64];
    num8lora_sender_receiver_slot_t slots[8];

    num8lora_receiver_t r1, r2;
    state_t s1={0}, s2={0};

    uint8_t beacon[64], req[64], tx[128], resp[128];
    uint32_t bl=0, rl=0, tl=0, rpl=0;
    uint16_t target=0;
    uint32_t first=0,last=0;
    uint64_t now=0;
    int guard=0;

    num8lora_sender_init(&snd,10u,1001u,50u,3u,ops,64u,slots,8u);
    C(num8lora_sender_register_receiver(&snd,21u,0u));
    C(num8lora_sender_register_receiver(&snd,22u,0u));

    num8lora_receiver_init(&r1,21u,10u,1001u,0u);
    num8lora_receiver_init(&r2,22u,10u,1001u,0u);

    C(num8lora_sender_enqueue_lists(&snd,(uint32_t[]){5u},1u,(uint32_t[]){7u,9u},2u,&first,&last));
    C(first==1u && last==3u);

    C(num8lora_sender_build_beacon(&snd,beacon,sizeof(beacon),&bl));

    C(num8lora_receiver_handle_beacon(&r1,beacon,bl,req,sizeof(req),&rl));
    C(num8lora_sender_handle_rx(&snd,now,req,rl));
    C(num8lora_receiver_handle_beacon(&r2,beacon,bl,req,sizeof(req),&rl));
    C(num8lora_sender_handle_rx(&snd,now,req,rl));

    while(guard++<200)
    {
        if(!num8lora_sender_poll_tx(&snd,now,tx,sizeof(tx),&tl,&target))
        {
            now+=10;
            continue;
        }

        if(target==21u)
        {
            C(num8lora_receiver_handle_data(&r1,tx,tl,apply_fn,&s1,resp,sizeof(resp),&rpl));
            C(num8lora_sender_handle_rx(&snd,now,resp,rpl));
        }
        else if(target==22u)
        {
            if(now<30u)
            {
                now+=60;
                continue;
            }
            C(num8lora_receiver_handle_data(&r2,tx,tl,apply_fn,&s2,resp,sizeof(resp),&rpl));
            C(num8lora_sender_handle_rx(&snd,now,resp,rpl));
        }

        now+=5;

        if(r1.last_applied_op_id==3u && r2.last_applied_op_id==3u)
        {
            break;
        }
    }

    C(r1.last_applied_op_id==3u);
    C(r2.last_applied_op_id==3u);
    C(s1.present[5]==0 && s1.present[7]==1 && s1.present[9]==1);
    C(s2.present[5]==0 && s2.present[7]==1 && s2.present[9]==1);

    printf("num8lora_async_test OK\n");
    return 0;
}
