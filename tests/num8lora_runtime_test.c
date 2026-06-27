#include "num8lora.h"

#include <stdio.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d\n", __LINE__); return 1; } } while (0)

static int test_atomicity_and_duplicates(void)
{
    num8lora_sender_ctx_t s;
    num8lora_receiver_ctx_t r;
    num8lora_update_header_t h;
    uint32_t rem[2] = {101u, 202u};
    uint32_t add[2] = {303u, 404u};
    uint32_t rem_alt[2] = {101u, 999u};
    uint32_t add_alt[2] = {303u, 405u};

    uint8_t beacon[64], req[64], upd[512], upd_dup[512], upd_bad[512], ack[64], tiny[17];
    uint32_t beacon_len = 0, req_len = 0, upd_len = 0, upd_dup_len = 0, upd_bad_len = 0, ack_len = 0, tiny_len = 0;
    num8lora_sender_ack_result_t ack_res = NUM8LORA_ACKRES_NONE;
    uint16_t nack_err = 0;
    uint16_t err = 0;
    num8lora_update_header_t val_h = {0};
    uint32_t required_remove = 0u;
    uint32_t required_add = 0u;
    uint32_t out_rem[2] = {0u, 0u};
    uint32_t out_add[2] = {0u, 0u};

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

    CHECK(!num8lora_receiver_handle_beacon(&r, beacon, beacon_len, req, 10u, &req_len));
    CHECK(r.state == NUM8LORA_RECEIVER_SCAN && req_len == 0u);
    CHECK(num8lora_receiver_handle_beacon(&r, beacon, beacon_len, req, sizeof(req), &req_len));
    CHECK(r.state == NUM8LORA_RECEIVER_WAIT_UPDATE);

    CHECK(!num8lora_sender_handle_update_request(&s, req, req_len, upd, 20u, &upd_len));
    CHECK(s.state == NUM8LORA_SENDER_WAIT_REQUEST && upd_len == 0u);
    CHECK(num8lora_sender_handle_update_request(&s, req, req_len, upd, sizeof(upd), &upd_len));
    CHECK(s.state == NUM8LORA_SENDER_WAIT_ACK);

    CHECK(num8lora_sender_on_ack_timeout(&s, upd_dup, sizeof(upd_dup), &upd_dup_len));
    CHECK(upd_dup_len == upd_len);

    CHECK(!num8lora_receiver_encode_ack(&r, 10u, 101u, tiny, sizeof(tiny), &tiny_len));
    CHECK(tiny_len == 0u);

    {
        num8lora_receiver_ctx_t ack_ctx = r;
        ack_ctx.local_dataset_version = 8u;
        CHECK(num8lora_receiver_encode_ack(&ack_ctx, 10u, 101u, ack, sizeof(ack), &ack_len));
    }
    CHECK(num8lora_sender_handle_ack_or_nack(&s, ack, ack_len, &ack_res, &nack_err));
    CHECK(ack_res == NUM8LORA_ACKRES_APPLIED);
    CHECK(s.state == NUM8LORA_SENDER_IDLE);

    CHECK(num8lora_receiver_validate_update_data(&r, upd, upd_len, &val_h, out_rem, 2u, out_add, 2u, &required_remove, &required_add, &err));
    CHECK(required_remove == 2u && required_add == 2u);
    CHECK(val_h.update_id == 101u);
    CHECK(num8lora_receiver_validate_update_data(&r, upd, upd_len, &val_h, out_rem, 2u, out_add, 2u, &required_remove, &required_add, &err));
    CHECK(required_remove == 2u && required_add == 2u);

    h.dataset_version_to = 9u;
    CHECK(num8lora_encode_update_data(upd_bad, sizeof(upd_bad), 10u, 22u, 5u, &h, rem, add, &upd_bad_len));
    CHECK(!num8lora_receiver_validate_update_data(&r, upd_bad, upd_bad_len, &val_h, out_rem, 2u, out_add, 2u, &required_remove, &required_add, &err));
    CHECK(err == NUM8LORA_ERR_UPDATE_ID_UNKNOWN || err == NUM8LORA_ERR_DATASET_VERSION_MISMATCH);

    h.dataset_version_to = 8u;
    h.remove_count = 1u;
    h.add_count = 1u;
    CHECK(num8lora_encode_update_data(upd_bad, sizeof(upd_bad), 10u, 22u, 6u, &h, rem_alt, add_alt, &upd_bad_len));
    CHECK(!num8lora_receiver_validate_update_data(&r, upd_bad, upd_bad_len, &val_h, out_rem, 2u, out_add, 2u, &required_remove, &required_add, &err));
    CHECK(err == NUM8LORA_ERR_UPDATE_ID_UNKNOWN);

    CHECK(num8lora_encode_update_data(upd_bad, sizeof(upd_bad), 10u, 23u, 7u, &h, rem, add, &upd_bad_len));
    CHECK(!num8lora_receiver_validate_update_data(&r, upd_bad, upd_bad_len, &val_h, out_rem, 2u, out_add, 2u, &required_remove, &required_add, &err));
    CHECK(err == NUM8LORA_ERR_INVALID_RECEIVER_ID);

    CHECK(num8lora_encode_update_data(upd_bad, sizeof(upd_bad), 11u, 22u, 8u, &h, rem, add, &upd_bad_len));
    CHECK(!num8lora_receiver_validate_update_data(&r, upd_bad, upd_bad_len, &val_h, out_rem, 2u, out_add, 2u, &required_remove, &required_add, &err));
    CHECK(err == NUM8LORA_ERR_INVALID_RECEIVER_ID);

    {
        uint16_t seq_before = r.seq;
        CHECK(!num8lora_receiver_encode_ack(&r, 10u, 101u, tiny, sizeof(tiny), &tiny_len));
        CHECK(tiny_len == 0u);
        CHECK(r.seq == seq_before);
    }

    {
        uint16_t seq_before = r.seq;
        CHECK(!num8lora_receiver_encode_nack(&r, 10u, 101u, NUM8LORA_ERR_INTERNAL, 0u, tiny, sizeof(tiny), &tiny_len));
        CHECK(tiny_len == 0u);
        CHECK(r.seq == seq_before);
    }

    CHECK(num8lora_receiver_encode_nack(&r, 10u, 101u, NUM8LORA_ERR_INTERNAL, 0u, ack, sizeof(ack), &ack_len));
    CHECK(ack_len == 18u);

    return 0;
}

int main(void)
{
    CHECK(test_atomicity_and_duplicates() == 0);

    printf("num8lora_runtime_test OK\n");
    return 0;
}
