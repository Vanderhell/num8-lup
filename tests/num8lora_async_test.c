#include "num8lora_sender.h"
#include "num8lora_receiver.h"

#include <stdio.h>

#define C(x) do { if(!(x)) { fprintf(stderr,"FAIL %d\n",__LINE__); return 1; } } while(0)

static void set_u16(uint8_t* p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static void refresh_async_crc(uint8_t* frame, uint32_t len)
{
    set_u16(&frame[len - 2u], num8lora_op_crc16_ccitt_false(frame, len - 2u));
}

typedef struct state_s { int present[1000]; uint32_t apply_calls; } state_t;

static int apply_fn(void* u, uint8_t op_type, uint32_t value)
{
    state_t* s=(state_t*)u;
    if(value>=1000u) return 0;
    s->apply_calls++;
    if(op_type==NUM8LORA_OP_ADD) s->present[value]=1;
    else if(op_type==NUM8LORA_OP_REMOVE) s->present[value]=0;
    else return 0;
    return 1;
}

static int fail_apply_fn(void* u, uint8_t op_type, uint32_t value)
{
    (void)u;
    (void)op_type;
    (void)value;
    return 0;
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

    set_u16(out + 2, 0u);
    set_u16(out + 12, 0u);
    refresh_async_crc(out, out_len);
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
    C(target == 22u);
    C(slots[1].waiting_ack == 1u && slots[1].inflight_op_id == 1u);

    C(num8lora_op_encode_ack(out, sizeof(out), 21u, 10u, 5u, &(num8lora_op_ack_payload_t){ snd.stream_id, 1u }, &out_len));
    C(!num8lora_sender_handle_rx(&snd, now, out, out_len));
    C(slots[0].waiting_ack == 0u && slots[0].inflight_op_id == 0u && slots[0].last_acked_op_id == 1u);
    C(slots[1].waiting_ack == 1u && slots[1].inflight_op_id == 1u);

    C(num8lora_op_encode_ack(out, sizeof(out), 22u, 10u, 6u, &(num8lora_op_ack_payload_t){ snd.stream_id, 1u }, &out_len));
    C(num8lora_sender_handle_rx(&snd, now, out, out_len));
    C(slots[1].waiting_ack == 0u && slots[1].inflight_op_id == 0u && slots[1].last_acked_op_id == 1u);

    C(num8lora_sender_poll_tx(&snd, now, out, sizeof(out), &sent_len, &target));
    C(target == 21u);
    C(slots[0].waiting_ack == 1u && slots[0].inflight_op_id == 2u);

    C(num8lora_op_encode_ack(out, sizeof(out), 21u, 10u, 7u, &(num8lora_op_ack_payload_t){ snd.stream_id, 2u }, &out_len));
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

static int test_sender_init_and_transactional_enqueue(void)
{
    num8lora_sender_t snd;
    num8lora_sender_op_t ops[3];
    num8lora_sender_receiver_slot_t slots[2];
    uint32_t out_first = 123u;
    uint32_t out_last = 456u;
    uint32_t op_id = 0u;
    uint32_t before_latest = 0u;

    num8lora_sender_init(&snd, 10u, 6001u, 25u, 2u, NULL, 1u, slots, 2u);
    C(snd.ops == NULL && snd.op_capacity == 0u && snd.receivers == NULL && snd.receiver_capacity == 0u);
    C(!num8lora_sender_enqueue_add(&snd, 1u, &op_id));
    C(op_id == 0u);

    num8lora_sender_init(&snd, 10u, 6001u, 25u, 2u, ops, 3u, NULL, 1u);
    C(snd.ops == NULL && snd.op_capacity == 0u && snd.receivers == NULL && snd.receiver_capacity == 0u);
    C(!num8lora_sender_enqueue_remove(&snd, 1u, &op_id));
    C(op_id == 0u);

    num8lora_sender_init(&snd, 10u, 6001u, 25u, 2u, ops, 3u, slots, 2u);
    C(snd.ops == ops && snd.op_capacity == 3u);
    C(snd.receivers == slots && snd.receiver_capacity == 2u);
    C(num8lora_sender_register_receiver(&snd, 21u, 0u));

    out_first = 99u;
    out_last = 88u;
    C(num8lora_sender_enqueue_lists(&snd, NULL, 0u, NULL, 0u, &out_first, &out_last));
    C(out_first == 0u && out_last == 0u);
    C(snd.latest_op_id == 0u && snd.op_count == 0u);

    out_first = 99u;
    out_last = 88u;
    C(!num8lora_sender_enqueue_lists(&snd, NULL, 1u, (uint32_t[]){ 7u }, 0u, &out_first, &out_last));
    C(out_first == 0u && out_last == 0u);
    C(snd.latest_op_id == 0u && snd.op_count == 0u);

    out_first = 99u;
    out_last = 88u;
    C(!num8lora_sender_enqueue_lists(&snd, (uint32_t[]){ 1u }, 0u, NULL, 1u, &out_first, &out_last));
    C(out_first == 0u && out_last == 0u);
    C(snd.latest_op_id == 0u && snd.op_count == 0u);

    out_first = 99u;
    out_last = 88u;
    C(!num8lora_sender_enqueue_lists(&snd, (uint32_t[]){ 1u }, 0u, (uint32_t[]){ 1u, 100000000u }, 2u, &out_first, &out_last));
    C(out_first == 0u && out_last == 0u);
    C(snd.latest_op_id == 0u && snd.op_count == 0u);

    out_first = 99u;
    out_last = 88u;
    C(!num8lora_sender_enqueue_lists(&snd, (uint32_t[]){ 1u, 2u }, 2u, (uint32_t[]){ 3u, 4u }, 2u, &out_first, &out_last));
    C(out_first == 0u && out_last == 0u);
    C(snd.latest_op_id == 0u && snd.op_count == 0u);

    out_first = 99u;
    out_last = 88u;
    C(num8lora_sender_enqueue_lists(&snd, (uint32_t[]){ 1u, 2u }, 2u, (uint32_t[]){ 3u }, 1u, &out_first, &out_last));
    C(out_first == 1u && out_last == 3u);
    C(snd.latest_op_id == 3u && snd.op_count == 3u);

    before_latest = snd.latest_op_id;
    C(!num8lora_sender_enqueue_lists(&snd, (uint32_t[]){ 5u }, 1u, (uint32_t[]){ 6u }, 1u, &out_first, &out_last));
    C(out_first == 0u && out_last == 0u);
    C(snd.latest_op_id == before_latest && snd.op_count == 3u);

    num8lora_sender_init(&snd, 10u, 6001u, 25u, 2u, ops, 3u, slots, 2u);
    snd.latest_op_id = UINT32_MAX;
    snd.oldest_retained_op_id = UINT32_MAX;
    snd.op_count = 0u;
    C(!num8lora_sender_enqueue_lists(&snd, (uint32_t[]){ 1u }, 1u, NULL, 0u, &out_first, &out_last));
    C(out_first == 0u && out_last == 0u);
    C(snd.latest_op_id == UINT32_MAX && snd.op_count == 0u);

    return 0;
}

static int test_receiver_apply_transaction_semantics(void)
{
    num8lora_receiver_t rx;
    state_t s = {0};
    uint8_t data[128];
    uint8_t resp[32];
    uint32_t data_len = 0u;
    uint32_t resp_len = 0u;
    num8lora_op_data_payload_t d = {0};
    num8lora_op_ack_payload_t ack = {0};
    num8lora_op_nack_payload_t nack = {0};
    num8lora_op_common_header_t hdr = {0};

    num8lora_receiver_init(&rx, 21u, 10u, 7001u, 0u);

    d.stream_id = rx.stream_id;
    d.op_id = 1u;
    d.op_type = NUM8LORA_OP_ADD;
    d.value = 5u;
    C(num8lora_op_encode_data(data, sizeof(data), 10u, 21u, 1u, &d, &data_len));

    resp_len = 0u;
    C(!num8lora_receiver_handle_data(&rx, data, data_len, apply_fn, &s, resp, 17u, &resp_len));
    C(resp_len == 0u && rx.last_applied_op_id == 0u && s.apply_calls == 0u);

    resp_len = 0u;
    C(num8lora_receiver_handle_data(&rx, data, data_len, NULL, &s, resp, sizeof(resp), &resp_len));
    C(resp_len == 22u);
    C(rx.last_applied_op_id == 0u && s.apply_calls == 0u);

    resp_len = 0u;
    C(num8lora_receiver_handle_data(&rx, data, data_len, fail_apply_fn, &s, resp, sizeof(resp), &resp_len));
    C(num8lora_op_decode_nack(resp, resp_len, &hdr, &nack));
    C(nack.error_code == NUM8LORA_OP_ERR_APPLY_FAILED);
    C(rx.last_applied_op_id == 0u && s.apply_calls == 0u);

    resp_len = 0u;
    C(num8lora_receiver_handle_data(&rx, data, data_len, apply_fn, &s, resp, sizeof(resp), &resp_len));
    C(num8lora_op_decode_ack(resp, resp_len, &hdr, &ack));
    C(ack.ack_op_id == 1u);
    C(rx.last_applied_op_id == 1u && s.present[5] == 1 && s.apply_calls == 1u);

    resp_len = 0u;
    C(num8lora_receiver_handle_data(&rx, data, data_len, apply_fn, &s, resp, sizeof(resp), &resp_len));
    C(num8lora_op_decode_ack(resp, resp_len, &hdr, &ack));
    C(ack.ack_op_id == 1u);
    C(rx.last_applied_op_id == 1u && s.present[5] == 1 && s.apply_calls == 1u);

    C(num8lora_op_encode_data(data, sizeof(data), 10u, 21u, 2u, &(num8lora_op_data_payload_t){ rx.stream_id, 1u, NUM8LORA_OP_ADD, 0u, 0u, 6u }, &data_len));
    resp_len = 0u;
    C(num8lora_receiver_handle_data(&rx, data, data_len, apply_fn, &s, resp, sizeof(resp), &resp_len));
    C(num8lora_op_decode_ack(resp, resp_len, &hdr, &ack));
    C(ack.ack_op_id == 1u);
    C(rx.last_applied_op_id == 1u && s.present[6] == 0 && s.apply_calls == 1u);

    C(num8lora_op_encode_data(data, sizeof(data), 10u, 21u, 3u, &(num8lora_op_data_payload_t){ rx.stream_id, 2u, NUM8LORA_OP_ADD, 0u, 0u, 6u }, &data_len));
    resp_len = 0u;
    C(num8lora_receiver_handle_data(&rx, data, data_len, apply_fn, &s, resp, sizeof(resp), &resp_len));
    C(num8lora_op_decode_ack(resp, resp_len, &hdr, &ack));
    C(ack.ack_op_id == 2u);
    C(rx.last_applied_op_id == 2u && s.present[6] == 1 && s.apply_calls == 2u);

    resp_len = 0u;
    C(num8lora_receiver_handle_data(&rx, data, data_len, apply_fn, &s, resp, sizeof(resp), &resp_len));
    C(num8lora_op_decode_ack(resp, resp_len, &hdr, &ack));
    C(ack.ack_op_id == 2u);
    C(rx.last_applied_op_id == 2u && s.present[6] == 1 && s.apply_calls == 2u);

    return 0;
}

static int test_operation_history_lifecycle(void)
{
    num8lora_sender_t snd;
    num8lora_sender_op_t ops[2];
    num8lora_sender_receiver_slot_t slots[1];
    uint8_t tx[128];
    uint32_t tx_len = 0;
    uint16_t target = 0;
    num8lora_op_ack_payload_t ack = {0};
    uint32_t op_id = 0;

    num8lora_sender_init(&snd, 10u, 2001u, 25u, 2u, ops, 2u, slots, 1u);
    C(num8lora_sender_register_receiver(&snd, 21u, 0u));

    C(num8lora_sender_enqueue_add(&snd, 11u, &op_id));
    C(op_id == 1u);
    C(num8lora_sender_enqueue_add(&snd, 12u, &op_id));
    C(op_id == 2u);
    C(!num8lora_sender_enqueue_add(&snd, 13u, &op_id));

    C(num8lora_sender_poll_tx(&snd, 0u, tx, sizeof(tx), &tx_len, &target));
    C(target == 21u && slots[0].inflight_op_id == 1u);
    ack.stream_id = snd.stream_id;
    ack.ack_op_id = 1u;
    C(num8lora_op_encode_ack(tx, sizeof(tx), 21u, 10u, 1u, &ack, &tx_len));
    C(num8lora_sender_handle_rx(&snd, 0u, tx, tx_len));
    C(snd.oldest_retained_op_id == 2u);
    C(snd.op_count == 1u);
    C(snd.op_head == 1u);

    C(num8lora_sender_enqueue_add(&snd, 13u, &op_id));
    C(op_id == 3u);
    C(snd.latest_op_id == 3u);
    C(snd.oldest_retained_op_id == 2u);
    C(snd.op_count == 2u);
    C(snd.op_head == 1u);

    C(num8lora_sender_poll_tx(&snd, 30u, tx, sizeof(tx), &tx_len, &target));
    C(target == 21u && slots[0].inflight_op_id == 2u);
    ack.ack_op_id = 2u;
    C(num8lora_op_encode_ack(tx, sizeof(tx), 21u, 10u, 2u, &ack, &tx_len));
    C(num8lora_sender_handle_rx(&snd, 30u, tx, tx_len));

    C(num8lora_sender_poll_tx(&snd, 60u, tx, sizeof(tx), &tx_len, &target));
    C(target == 21u && slots[0].inflight_op_id == 3u);
    ack.ack_op_id = 3u;
    C(num8lora_op_encode_ack(tx, sizeof(tx), 21u, 10u, 3u, &ack, &tx_len));
    C(num8lora_sender_handle_rx(&snd, 60u, tx, tx_len));

    C(snd.op_count == 0u);
    C(snd.oldest_retained_op_id == 4u);

    C(num8lora_sender_enqueue_add(&snd, 14u, &op_id));
    C(op_id == 4u);
    C(snd.op_count == 1u);
    C(snd.oldest_retained_op_id == 4u);

    return 0;
}

static int test_slow_and_inactive_receiver_policy(void)
{
    num8lora_sender_t snd;
    num8lora_sender_op_t ops[2];
    num8lora_sender_receiver_slot_t slots[2];
    uint32_t op_id = 0;
    uint8_t tx[128];
    uint32_t tx_len = 0;
    uint16_t target = 0;
    num8lora_op_ack_payload_t ack = {0};

    num8lora_sender_init(&snd, 10u, 3001u, 25u, 2u, ops, 2u, slots, 2u);
    C(num8lora_sender_register_receiver(&snd, 21u, 0u));
    C(num8lora_sender_register_receiver(&snd, 22u, 0u));

    C(num8lora_sender_enqueue_add(&snd, 21u, &op_id));
    C(num8lora_sender_enqueue_add(&snd, 22u, &op_id));
    C(num8lora_sender_poll_tx(&snd, 0u, tx, sizeof(tx), &tx_len, &target));
    C(target == 21u);
    ack.stream_id = snd.stream_id;
    ack.ack_op_id = 1u;
    C(num8lora_op_encode_ack(tx, sizeof(tx), 21u, 10u, 1u, &ack, &tx_len));
    C(num8lora_sender_handle_rx(&snd, 0u, tx, tx_len));
    C(!num8lora_sender_enqueue_add(&snd, 23u, &op_id));

    C(num8lora_sender_unregister_receiver(&snd, 22u));
    C(num8lora_sender_enqueue_add(&snd, 23u, &op_id));
    C(op_id == 3u);
    return 0;
}

static int test_history_nack_and_replay(void)
{
    num8lora_sender_t snd;
    num8lora_sender_op_t ops[2];
    num8lora_sender_receiver_slot_t slots[1];
    uint8_t req[128];
    uint32_t req_len = 0;
    uint8_t nack_buf[128];
    uint32_t nack_len = 0;
    uint16_t err = 0;
    num8lora_op_common_header_t hdr = {0};
    num8lora_op_nack_payload_t nack = {0};

    num8lora_sender_init(&snd, 10u, 4001u, 25u, 2u, ops, 2u, slots, 1u);
    C(num8lora_sender_register_receiver(&snd, 21u, 0u));
    C(num8lora_sender_enqueue_add(&snd, 31u, NULL));
    C(num8lora_sender_enqueue_add(&snd, 32u, NULL));

    C(num8lora_sender_poll_tx(&snd, 0u, req, sizeof(req), &req_len, &err));
    C(num8lora_op_encode_ack(req, sizeof(req), 21u, 10u, 1u, &(num8lora_op_ack_payload_t){ snd.stream_id, 1u }, &req_len));
    C(num8lora_sender_handle_rx(&snd, 0u, req, req_len));

    C(num8lora_op_encode_request(req, sizeof(req), 21u, 10u, 2u, &(num8lora_op_request_payload_t){ snd.stream_id, 1u }, &req_len));
    C(num8lora_sender_handle_request(&snd, req, req_len, nack_buf, sizeof(nack_buf), &nack_len));
    C(num8lora_op_decode_nack(nack_buf, nack_len, &hdr, &nack));
    C(nack.error_code == NUM8LORA_OP_ERR_HISTORY_UNAVAILABLE);
    C(nack.expected_next_op_id == snd.oldest_retained_op_id);

    C(num8lora_sender_poll_tx(&snd, 30u, req, sizeof(req), &req_len, &err));
    C(err == 21u);
    C(num8lora_op_encode_ack(req, sizeof(req), 21u, 10u, 3u, &(num8lora_op_ack_payload_t){ snd.stream_id, 1u }, &req_len));
    C(!num8lora_sender_handle_rx(&snd, 30u, req, req_len));
    C(slots[0].waiting_ack == 1u && slots[0].inflight_op_id == 2u && slots[0].last_acked_op_id == 1u);

    C(num8lora_op_encode_ack(req, sizeof(req), 21u, 10u, 4u, &(num8lora_op_ack_payload_t){ snd.stream_id, 1u }, &req_len));
    C(!num8lora_sender_handle_rx(&snd, 30u, req, req_len));
    C(num8lora_op_encode_nack(req, sizeof(req), 21u, 10u, 5u, &(num8lora_op_nack_payload_t){ snd.stream_id, NUM8LORA_OP_ERR_SEQUENCE_GAP, 0u, 99u }, &req_len));
    C(!num8lora_sender_handle_rx(&snd, 30u, req, req_len));
    C(num8lora_op_encode_request(req, sizeof(req), 21u, 10u, 6u, &(num8lora_op_request_payload_t){ snd.stream_id, 1u }, &req_len));
    C(!num8lora_sender_handle_rx(&snd, 30u, req, req_len));
    C(slots[0].waiting_ack == 1u && slots[0].inflight_op_id == 2u);

    return 0;
}

static int test_retry_fairness_and_timeout_edges(void)
{
    num8lora_sender_t snd;
    num8lora_sender_op_t ops[1];
    num8lora_sender_receiver_slot_t slots[2];
    uint8_t tx[128];
    uint32_t tx_len = 0;
    uint16_t target = 0;

    num8lora_sender_init(&snd, 10u, 5001u, 50u, 1u, ops, 1u, slots, 2u);
    C(num8lora_sender_register_receiver(&snd, 21u, 0u));
    C(num8lora_sender_register_receiver(&snd, 22u, 0u));
    C(num8lora_sender_enqueue_add(&snd, 41u, NULL));

    C(num8lora_sender_poll_tx(&snd, 0u, tx, sizeof(tx), &tx_len, &target));
    C(target == 21u);
    C(slots[0].waiting_ack == 1u);

    C(num8lora_sender_poll_tx(&snd, 100u, tx, sizeof(tx), &tx_len, &target));
    C(target == 22u);

    num8lora_sender_init(&snd, 10u, 5002u, 0u, 2u, ops, 1u, slots, 2u);
    C(num8lora_sender_register_receiver(&snd, 21u, 0u));
    C(num8lora_sender_enqueue_add(&snd, 42u, NULL));
    C(num8lora_sender_poll_tx(&snd, 55u, tx, sizeof(tx), &tx_len, &target));
    C(target == 21u);
    C(slots[0].due_at_ms == 55u);
    C(num8lora_sender_poll_tx(&snd, 55u, tx, sizeof(tx), &tx_len, &target));
    C(target == 21u);

    num8lora_sender_init(&snd, 10u, 5003u, 25u, 1u, ops, 1u, slots, 1u);
    C(num8lora_sender_register_receiver(&snd, 21u, 0u));
    C(num8lora_sender_enqueue_add(&snd, 43u, NULL));
    C(num8lora_sender_poll_tx(&snd, UINT64_MAX - 5u, tx, sizeof(tx), &tx_len, &target));
    C(target == 21u);
    C(slots[0].due_at_ms == UINT64_MAX);
    C(!num8lora_sender_poll_tx(&snd, UINT64_MAX - 5u, tx, sizeof(tx), &tx_len, &target));

    num8lora_sender_init(&snd, 10u, 5004u, 0u, 1u, ops, 1u, slots, 1u);
    C(num8lora_sender_register_receiver(&snd, 21u, 0u));
    C(num8lora_sender_enqueue_add(&snd, 44u, NULL));
    C(num8lora_sender_poll_tx(&snd, 10u, tx, sizeof(tx), &tx_len, &target));
    C(target == 21u);
    C(num8lora_sender_poll_tx(&snd, 10u, tx, sizeof(tx), &tx_len, &target));
    C(target == 21u);
    C(!num8lora_sender_poll_tx(&snd, 10u, tx, sizeof(tx), &tx_len, &target));
    C(slots[0].exhausted == 1u);
    C(!num8lora_sender_poll_tx(&snd, 10u, tx, sizeof(tx), &tx_len, &target));

    return 0;
}

int main(void)
{
    C(test_sender_routing_gates() == 0);
    C(test_receiver_routing_gates() == 0);
    C(test_sender_init_and_transactional_enqueue() == 0);
    C(test_receiver_apply_transaction_semantics() == 0);
    C(test_operation_history_lifecycle() == 0);
    C(test_slow_and_inactive_receiver_policy() == 0);
    C(test_history_nack_and_replay() == 0);
    C(test_retry_fairness_and_timeout_edges() == 0);

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
