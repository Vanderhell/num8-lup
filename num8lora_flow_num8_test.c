#include "num8lora.h"
#include "num8.h"

#include <stdio.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d\n", __LINE__); return 1; } } while (0)

int main(void)
{
    const char* f = "lora_flow_tmp.num8";
    num8_engine_t e;
    num8_status_t st;
    num8lora_sender_ctx_t s;
    num8lora_receiver_ctx_t r;
    num8lora_update_header_t h;
    uint32_t rem[1] = {100u};
    uint32_t add[2] = {200u, 300u};

    uint8_t beacon[64], req[64], upd[512], resp[64], resp2[64];
    uint32_t beacon_len = 0, req_len = 0, upd_len = 0, resp_len = 0, resp2_len = 0;
    uint16_t err = 0;
    num8lora_sender_ack_result_t ack_res = NUM8LORA_ACKRES_NONE;
    uint16_t nack_err = 0;
    int ex = 0;

    st = num8_create(f, &e);
    CHECK(st == NUM8_STATUS_OK);
    CHECK(num8_add_u32(&e, 100u) == NUM8_STATUS_ADDED);
    CHECK(num8_flush(&e) == NUM8_STATUS_OK);

    num8lora_sender_init(&s, 10u, 3u);
    num8lora_receiver_init(&r, 22u, 10u, 3u, 7u, 99u);

    h.update_id = 101u;
    h.dataset_version_from = 7u;
    h.dataset_version_to = 8u;
    h.remove_count = 1u;
    h.add_count = 2u;
    h.reserved0 = 0u;

    CHECK(num8lora_sender_set_update(&s, 22u, &h, rem, add, &err));
    CHECK(num8lora_sender_build_beacon(&s, beacon, sizeof(beacon), &beacon_len));
    CHECK(num8lora_receiver_handle_beacon(&r, beacon, beacon_len, req, sizeof(req), &req_len));
    CHECK(num8lora_sender_handle_update_request(&s, req, req_len, upd, sizeof(upd), &upd_len));

    CHECK(num8lora_receiver_apply_update_data_to_num8(&r, &e, upd, upd_len, resp, sizeof(resp), &resp_len, &err));
    CHECK(err == NUM8LORA_ERR_NONE);
    CHECK(r.local_dataset_version == 8u);
    CHECK(r.last_applied_update_id == 101u);

    CHECK(num8lora_sender_handle_ack_or_nack(&s, resp, resp_len, &ack_res, &nack_err));
    CHECK(ack_res == NUM8LORA_ACKRES_APPLIED);

    CHECK(num8_exists_u32(&e, 100u, &ex) == NUM8_STATUS_OK && ex == 0);
    CHECK(num8_exists_u32(&e, 200u, &ex) == NUM8_STATUS_OK && ex == 1);
    CHECK(num8_exists_u32(&e, 300u, &ex) == NUM8_STATUS_OK && ex == 1);

    CHECK(num8lora_receiver_apply_update_data_to_num8(&r, &e, upd, upd_len, resp2, sizeof(resp2), &resp2_len, &err));
    CHECK(err == NUM8LORA_ERR_NONE);

    CHECK(num8_close(&e) == NUM8_STATUS_OK);
    remove(f);

    printf("num8lora_flow_num8_test OK\n");
    return 0;
}
