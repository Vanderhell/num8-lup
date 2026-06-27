#include "num8lora.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d\n", __LINE__); return 1; } } while (0)

static int beacon_payload_equal(const num8lora_beacon_payload_t* a, const num8lora_beacon_payload_t* b)
{
    return a->current_update_id == b->current_update_id
        && a->dataset_version_from == b->dataset_version_from
        && a->dataset_version_to == b->dataset_version_to
        && a->remove_count == b->remove_count
        && a->add_count == b->add_count
        && a->update_payload_size == b->update_payload_size
        && a->update_payload_crc16 == b->update_payload_crc16;
}

static int test_beacon(void)
{
    uint8_t buf[64];
    uint32_t len = 0;
    num8lora_beacon_payload_t in_pl;
    num8lora_beacon_payload_t out_pl;
    num8lora_common_header_t hdr;

    in_pl.current_update_id = 10u;
    in_pl.dataset_version_from = 200u;
    in_pl.dataset_version_to = 201u;
    in_pl.remove_count = 2u;
    in_pl.add_count = 3u;
    in_pl.update_payload_size = 16u + 8u + 12u;
    in_pl.update_payload_crc16 = 0x9A23u;

    CHECK(num8lora_encode_beacon(buf, sizeof(buf), 77u, 5u, &in_pl, &len));
    CHECK(len == 28u);
    CHECK(num8lora_decode_beacon(buf, len, &hdr, &out_pl));
    CHECK(hdr.protocol_version == 1u);
    CHECK(hdr.msg_type == NUM8LORA_MSG_BEACON);
    CHECK(hdr.sender_id == 77u);
    CHECK(hdr.receiver_id == 0u);
    CHECK(hdr.seq == 5u);
    CHECK(beacon_payload_equal(&in_pl, &out_pl));

    buf[3] ^= 0x01u;
    CHECK(!num8lora_validate_frame_crc(buf, len));
    return 0;
}

static int test_update_data(void)
{
    uint8_t buf[256];
    uint32_t len = 0;
    num8lora_update_header_t h;
    num8lora_common_header_t out_hdr;
    num8lora_update_header_t out_h;
    const uint8_t* rp = NULL;
    const uint8_t* ap = NULL;
    uint32_t rem[2] = {123u, 456u};
    uint32_t add[3] = {789u, 1000u, 99999999u};

    h.update_id = 50u;
    h.dataset_version_from = 10u;
    h.dataset_version_to = 11u;
    h.remove_count = 2u;
    h.add_count = 3u;
    h.reserved0 = 0u;

    CHECK(num8lora_encode_update_data(buf, sizeof(buf), 1u, 2u, 9u, &h, rem, add, &len));
    CHECK(len == 26u + 8u + 12u);

    CHECK(num8lora_decode_update_data_header(buf, len, &out_hdr, &out_h, &rp, &ap));
    CHECK(out_hdr.msg_type == NUM8LORA_MSG_UPDATE_DATA);
    CHECK(out_h.update_id == h.update_id);
    CHECK(out_h.dataset_version_from == h.dataset_version_from);
    CHECK(out_h.dataset_version_to == h.dataset_version_to);
    CHECK(out_h.remove_count == h.remove_count);
    CHECK(out_h.add_count == h.add_count);
    CHECK(out_h.reserved0 == 0u);

    CHECK(((uint32_t)rp[0] | ((uint32_t)rp[1] << 8) | ((uint32_t)rp[2] << 16) | ((uint32_t)rp[3] << 24)) == rem[0]);
    CHECK(((uint32_t)ap[0] | ((uint32_t)ap[1] << 8) | ((uint32_t)ap[2] << 16) | ((uint32_t)ap[3] << 24)) == add[0]);
    return 0;
}

static int test_update_numbers_capacity(void)
{
    uint8_t buf[2200];
    uint32_t len = 0u;
    num8lora_update_header_t h;
    num8lora_common_header_t hdr;
    num8lora_update_header_t out_h;
    const uint8_t* rp = NULL;
    const uint8_t* ap = NULL;
    uint32_t rem[255];
    uint32_t add[255];
    uint32_t out_rem[255];
    uint32_t out_add[255];
    uint32_t required_remove = 0u;
    uint32_t required_add = 0u;
    uint16_t i;

    for (i = 0u; i < 255u; ++i)
    {
        rem[i] = 1000u + (uint32_t)i;
        add[i] = 2000u + (uint32_t)i;
    }

    h.update_id = 77u;
    h.dataset_version_from = 10u;
    h.dataset_version_to = 11u;
    h.reserved0 = 0u;

    h.remove_count = 0u;
    h.add_count = 0u;
    CHECK(num8lora_encode_update_data(buf, sizeof(buf), 1u, 2u, 9u, &h, NULL, NULL, &len));
    CHECK(len == 26u);
    CHECK(num8lora_decode_update_data_header(buf, len, &hdr, &out_h, &rp, &ap));
    CHECK(num8lora_decode_update_numbers(&out_h, rp, 0u, ap, 0u, NULL, 0u, NULL, 0u, &required_remove, &required_add));
    CHECK(required_remove == 0u && required_add == 0u);

    h.remove_count = 1u;
    h.add_count = 1u;
    CHECK(num8lora_encode_update_data(buf, sizeof(buf), 1u, 2u, 9u, &h, rem, add, &len));
    CHECK(len == 34u);
    CHECK(num8lora_decode_update_data_header(buf, len, &hdr, &out_h, &rp, &ap));
    out_rem[0] = 0u;
    out_add[0] = 0u;
    CHECK(!num8lora_decode_update_numbers(&out_h, rp, 1u, ap, 1u, out_rem, 0u, out_add, 1u, &required_remove, &required_add));
    CHECK(required_remove == 1u && required_add == 1u);
    CHECK(out_rem[0] == 0u && out_add[0] == 0u);
    CHECK(num8lora_decode_update_numbers(&out_h, rp, 1u, ap, 1u, out_rem, 1u, out_add, 1u, &required_remove, &required_add));
    CHECK(out_rem[0] == rem[0] && out_add[0] == add[0]);

    h.remove_count = 254u;
    h.add_count = 1u;
    CHECK(num8lora_encode_update_data(buf, sizeof(buf), 1u, 2u, 9u, &h, rem, add, &len));
    CHECK(len == 26u + 4u * 255u);
    CHECK(num8lora_decode_update_data_header(buf, len, &hdr, &out_h, &rp, &ap));
    CHECK(!num8lora_decode_update_numbers(&out_h, rp, 254u, ap, 1u, out_rem, 253u, out_add, 1u, &required_remove, &required_add));
    CHECK(required_remove == 254u && required_add == 1u);
    CHECK(num8lora_decode_update_numbers(&out_h, rp, 254u, ap, 1u, out_rem, 254u, out_add, 1u, &required_remove, &required_add));
    CHECK(out_rem[0] == rem[0] && out_rem[253] == rem[253] && out_add[0] == add[0]);

    h.remove_count = 255u;
    h.add_count = 255u;
    CHECK(num8lora_encode_update_data(buf, sizeof(buf), 1u, 2u, 9u, &h, rem, add, &len));
    CHECK(len == 2066u);
    CHECK(num8lora_decode_update_data_header(buf, len, &hdr, &out_h, &rp, &ap));
    CHECK(!num8lora_decode_update_numbers(&out_h, rp, 255u, ap, 255u, out_rem, 254u, out_add, 255u, &required_remove, &required_add));
    CHECK(required_remove == 255u && required_add == 255u);
    CHECK(num8lora_decode_update_numbers(&out_h, rp, 255u, ap, 255u, out_rem, 255u, out_add, 255u, &required_remove, &required_add));
    CHECK(out_rem[254] == rem[254] && out_add[254] == add[254]);

    return 0;
}

static int test_validate_lists(void)
{
    num8lora_update_header_t h;
    uint16_t err = 0;
    uint32_t r1[2] = {1u, 1u};
    uint32_t a1[1] = {2u};
    uint32_t r2[1] = {3u};
    uint32_t a2[2] = {3u, 4u};

    h.update_id = 1u;
    h.dataset_version_from = 1u;
    h.dataset_version_to = 2u;
    h.reserved0 = 0u;

    h.remove_count = 2u;
    h.add_count = 1u;
    CHECK(!num8lora_validate_update_numbers(&h, r1, a1, &err));
    CHECK(err == NUM8LORA_ERR_DUPLICATE_IN_BATCH);

    h.remove_count = 1u;
    h.add_count = 2u;
    CHECK(!num8lora_validate_update_numbers(&h, r2, a2, &err));
    CHECK(err == NUM8LORA_ERR_NUMBER_IN_BOTH);

    return 0;
}

static int test_failure_outputs(void)
{
    uint8_t buf[32] = {0};
    uint32_t len = 123u;
    num8lora_common_header_t hdr;
    num8lora_beacon_payload_t beacon = {1u, 2u, 3u, 4u, 5u, 6u, 7u};
    uint16_t err = 0xFFFFu;
    num8lora_update_request_payload_t req = {1u, 2u};

    CHECK(!num8lora_encode_beacon(buf, 10u, 1u, 1u, &beacon, &len));
    CHECK(len == 0u);

    CHECK(!num8lora_decode_common_header(buf, 0u, &hdr));
    CHECK(hdr.protocol_version == 0u && hdr.msg_type == 0u && hdr.sender_id == 0u && hdr.receiver_id == 0u && hdr.seq == 0u);

    CHECK(!num8lora_decode_beacon(buf, 0u, &hdr, &beacon));
    CHECK(beacon.current_update_id == 0u);
    CHECK(beacon.dataset_version_from == 0u);
    CHECK(beacon.dataset_version_to == 0u);
    CHECK(beacon.remove_count == 0u);
    CHECK(beacon.add_count == 0u);
    CHECK(beacon.update_payload_size == 0u);
    CHECK(beacon.update_payload_crc16 == 0u);

    CHECK(!num8lora_classify_frame(buf, 0u, 0u, 0u, &hdr, &err));
    CHECK(err == NUM8LORA_ERR_CRC_FAIL);
    CHECK(hdr.protocol_version == 0u && hdr.msg_type == 0u && hdr.sender_id == 0u && hdr.receiver_id == 0u && hdr.seq == 0u);

    CHECK(!num8lora_decode_update_request(buf, 0u, &hdr, &req));
    CHECK(req.requested_update_id == 0u && req.receiver_dataset_ver == 0u);

    return 0;
}

int main(void)
{
    CHECK(test_beacon() == 0);
    CHECK(test_update_data() == 0);
    CHECK(test_update_numbers_capacity() == 0);
    CHECK(test_validate_lists() == 0);
    CHECK(test_failure_outputs() == 0);
    printf("num8lora_test OK\n");
    return 0;
}
