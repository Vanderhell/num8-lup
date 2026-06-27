#include "num8lora.h"
#include "num8lora_op.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d\n", __LINE__); return 1; } } while (0)

static void set_u16(uint8_t* p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static void set_u32(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static void refresh_legacy_crc(uint8_t* frame, uint32_t len)
{
    set_u16(&frame[len - 2u], num8lora_crc16_ccitt_false(frame, len - 2u));
}

static void refresh_async_crc(uint8_t* frame, uint32_t len)
{
    set_u16(&frame[len - 2u], num8lora_op_crc16_ccitt_false(frame, len - 2u));
}

static int test_crc_vectors(void)
{
    static const uint8_t vector[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };

    CHECK(num8lora_crc16_ccitt_false(NULL, 0u) == 0xFFFFu);
    CHECK(num8lora_op_crc16_ccitt_false(NULL, 0u) == 0xFFFFu);
    CHECK(num8lora_crc16_ccitt_false(NULL, 3u) == 0u);
    CHECK(num8lora_op_crc16_ccitt_false(NULL, 3u) == 0u);
    CHECK(num8lora_crc16_ccitt_false(vector, (uint32_t)sizeof(vector)) == 0x29B1u);
    CHECK(num8lora_op_crc16_ccitt_false(vector, (uint32_t)sizeof(vector)) == 0x29B1u);
    return 0;
}

static int test_legacy_frame_validation(void)
{
    uint8_t frame[64];
    uint32_t len = 0u;
    num8lora_update_header_t hdr = { 10u, 1u, 2u, 1u, 1u, 0u };
    uint32_t rem[1] = { 7u };
    uint32_t add[1] = { 8u };
    num8lora_common_header_t out_hdr;
    num8lora_update_header_t out_upd;
    const uint8_t* rp = NULL;
    const uint8_t* ap = NULL;

    CHECK(num8lora_encode_update_data(frame, sizeof(frame), 11u, 22u, 3u, &hdr, rem, add, &len));
    CHECK(num8lora_decode_update_data_header(frame, len, &out_hdr, &out_upd, &rp, &ap));
    CHECK(out_hdr.protocol_version == NUM8LORA_PROTOCOL_VERSION);
    CHECK(out_hdr.msg_type == NUM8LORA_MSG_UPDATE_DATA);
    CHECK(out_hdr.sender_id == 11u);
    CHECK(out_hdr.receiver_id == 22u);
    CHECK(out_upd.update_id == hdr.update_id);
    CHECK(num8lora_validate_frame_crc(frame, len));

    frame[0] = 0x7Fu;
    refresh_legacy_crc(frame, len);
    CHECK(!num8lora_decode_update_data_header(frame, len, &out_hdr, &out_upd, &rp, &ap));
    CHECK(num8lora_validate_frame_crc(frame, len));

    CHECK(num8lora_encode_update_data(frame, sizeof(frame), 11u, 22u, 3u, &hdr, rem, add, &len));
    frame[1] = 0xFFu;
    refresh_legacy_crc(frame, len);
    CHECK(!num8lora_decode_update_data_header(frame, len, &out_hdr, &out_upd, &rp, &ap));

    CHECK(num8lora_encode_update_data(frame, sizeof(frame), 11u, 22u, 3u, &hdr, rem, add, &len));
    frame[22] = 1u;
    refresh_legacy_crc(frame, len);
    CHECK(!num8lora_decode_update_data_header(frame, len, &out_hdr, &out_upd, &rp, &ap));

    CHECK(num8lora_encode_update_data(frame, sizeof(frame), 11u, 22u, 3u, &hdr, rem, add, &len));
    CHECK(!num8lora_decode_update_data_header(frame, len - 1u, &out_hdr, &out_upd, &rp, &ap));

    CHECK(num8lora_validate_update_numbers(&hdr, rem, add, NULL));
    rem[0] = 100000000u;
    CHECK(!num8lora_validate_update_numbers(&hdr, rem, add, NULL));
    return 0;
}

static int test_async_frame_validation(void)
{
    uint8_t frame[64];
    uint32_t len = 0u;
    num8lora_op_data_payload_t data = { 77u, 33u, NUM8LORA_OP_ADD, 0u, 0u, 12345678u };
    num8lora_op_common_header_t out_hdr;
    num8lora_op_data_payload_t out_data;

    CHECK(num8lora_op_encode_data(frame, sizeof(frame), 1u, 2u, 4u, &data, &len));
    CHECK(num8lora_op_decode_data(frame, len, &out_hdr, &out_data));
    CHECK(out_hdr.protocol_version == NUM8LORA_OP_PROTOCOL_VERSION);
    CHECK(out_hdr.msg_type == NUM8LORA_OP_MSG_DATA);
    CHECK(out_hdr.sender_id == 1u);
    CHECK(out_hdr.receiver_id == 2u);
    CHECK(out_data.value == data.value);
    CHECK(num8lora_op_validate_frame_crc(frame, len));

    frame[16] = 0x7Fu;
    refresh_async_crc(frame, len);
    CHECK(!num8lora_op_decode_data(frame, len, &out_hdr, &out_data));
    CHECK(num8lora_op_validate_frame_crc(frame, len));

    CHECK(num8lora_op_encode_data(frame, sizeof(frame), 1u, 2u, 4u, &data, &len));
    frame[0] = 0x7Fu;
    refresh_async_crc(frame, len);
    CHECK(!num8lora_op_decode_data(frame, len, &out_hdr, &out_data));

    CHECK(num8lora_op_encode_data(frame, sizeof(frame), 1u, 2u, 4u, &data, &len));
    frame[1] = 0xFFu;
    refresh_async_crc(frame, len);
    CHECK(!num8lora_op_decode_data(frame, len, &out_hdr, &out_data));

    CHECK(num8lora_op_encode_data(frame, sizeof(frame), 1u, 2u, 4u, &data, &len));
    frame[17] = 1u;
    refresh_async_crc(frame, len);
    CHECK(!num8lora_op_decode_data(frame, len, &out_hdr, &out_data));

    CHECK(num8lora_op_encode_data(frame, sizeof(frame), 1u, 2u, 4u, &data, &len));
    set_u32(&frame[20], 100000000u);
    refresh_async_crc(frame, len);
    CHECK(!num8lora_op_decode_data(frame, len, &out_hdr, &out_data));

    CHECK(num8lora_op_encode_ack(frame, sizeof(frame), 1u, 2u, 4u, &(num8lora_op_ack_payload_t){77u, 33u}, &len));
    CHECK(!num8lora_op_decode_data(frame, len, &out_hdr, &out_data));
    return 0;
}

static int test_corpus_rejection(void)
{
    uint8_t legacy[64];
    uint8_t asyncf[64];
    uint32_t legacy_len = 0u;
    uint32_t async_len = 0u;
    num8lora_update_header_t legacy_hdr = { 44u, 2u, 3u, 1u, 2u, 0u };
    num8lora_op_data_payload_t async_data = { 90u, 91u, NUM8LORA_OP_REMOVE, 0u, 0u, 42u };
    uint32_t rem[2] = { 10u, 11u };
    uint32_t add[2] = { 12u, 13u };
    num8lora_common_header_t oh;
    num8lora_update_header_t ou;
    const uint8_t* rp = NULL;
    const uint8_t* ap = NULL;
    num8lora_op_common_header_t ah;
    num8lora_op_data_payload_t ad;

    CHECK(num8lora_encode_update_data(legacy, sizeof(legacy), 9u, 8u, 7u, &legacy_hdr, rem, add, &legacy_len));
    legacy[5] ^= 0x01u;
    CHECK(!num8lora_decode_update_data_header(legacy, legacy_len, &oh, &ou, &rp, &ap));

    CHECK(num8lora_op_encode_data(asyncf, sizeof(asyncf), 9u, 8u, 7u, &async_data, &async_len));
    asyncf[5] ^= 0x01u;
    CHECK(!num8lora_op_decode_data(asyncf, async_len, &ah, &ad));

    CHECK(!num8lora_decode_update_data_header(legacy, legacy_len - 2u, &oh, &ou, &rp, &ap));
    CHECK(!num8lora_op_decode_data(asyncf, async_len - 2u, &ah, &ad));
    return 0;
}

int main(void)
{
    CHECK(test_crc_vectors() == 0);
    CHECK(test_legacy_frame_validation() == 0);
    CHECK(test_async_frame_validation() == 0);
    CHECK(test_corpus_rejection() == 0);
    puts("num8lora_codec_contract_test OK");
    return 0;
}
