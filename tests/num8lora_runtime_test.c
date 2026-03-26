#include "num8lora.h"

#include <stdio.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d\n", __LINE__); return 1; } } while (0)

int main(void)
{
    num8lora_sender_ctx_t s;
    num8lora_receiver_ctx_t r;
    num8lora_update_header_t h;
    uint32_t rem[2] = {101u, 202u};
    uint32_t add[2] = {303u, 404u};

    uint8_t beacon[64], req[64], upd[512], ack[64], upd2[512];
    uint32_t beacon_len = 0, req_len = 0, upd_len = 0, ack_len = 0, upd2_len = 0;
    num8lora_sender_ack_result_t ack_res = NUM8LORA_ACKRES_NONE;
    uint16_t nack_err = 0;
    uint16_t err = 0;
    uint32_t dv = 0, lu = 0;

    num8lora_sender_init(&s, 10u, 3u);
    num8lora_receiver_init(&r, 22u, 10u, 3u, 7u, 100u);

    h.update_id = 101u;
    h.dataset_version_from = 7u;
    h.dataset_version_to = 8u;
    h.remove_count = 2u;
    h.add_count = 2u;
    h.reserved0 = 0u;

    CHECK(num8lora_sender_set_update(&s, 22u, &h, rem, add, &err));
    CHECK(num8lora_sender_build_beacon(&s, beacon, sizeof(beacon), &beacon_len));

    CHECK(num8lora_receiver_handle_beacon(&r, beacon, beacon_len, req, sizeof(req), &req_len));
    CHECK(r.state == NUM8LORA_RECEIVER_WAIT_UPDATE);

    CHECK(num8lora_sender_handle_update_request(&s, req, req_len, upd, sizeof(upd), &upd_len));
    CHECK(s.state == NUM8LORA_SENDER_WAIT_ACK);

    CHECK(num8lora_sender_on_ack_timeout(&s, upd2, sizeof(upd2), &upd2_len));
    CHECK(upd2_len == upd_len);

    r.local_dataset_version = 8u;
    CHECK(num8lora_receiver_encode_ack(&r, 10u, 101u, ack, sizeof(ack), &ack_len));

    CHECK(num8lora_sender_handle_ack_or_nack(&s, ack, ack_len, &ack_res, &nack_err));
    CHECK(ack_res == NUM8LORA_ACKRES_APPLIED);
    CHECK(s.state == NUM8LORA_SENDER_IDLE);

    CHECK(num8lora_save_receiver_meta("receiver_meta.tmp", 8u, 101u));
    CHECK(num8lora_load_receiver_meta("receiver_meta.tmp", &dv, &lu));
    CHECK(dv == 8u && lu == 101u);
    remove("receiver_meta.tmp");

    printf("num8lora_runtime_test OK\n");
    return 0;
}
