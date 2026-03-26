#ifndef NUM8LORA_OP_H
#define NUM8LORA_OP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NUM8LORA_OP_PROTOCOL_VERSION 1u

#define NUM8LORA_OP_MSG_BEACON   0x21u
#define NUM8LORA_OP_MSG_REQUEST  0x22u
#define NUM8LORA_OP_MSG_DATA     0x23u
#define NUM8LORA_OP_MSG_ACK      0x24u
#define NUM8LORA_OP_MSG_NACK     0x25u

#define NUM8LORA_OP_ADD          1u
#define NUM8LORA_OP_REMOVE       2u

#define NUM8LORA_OP_ERR_NONE                 0u
#define NUM8LORA_OP_ERR_CRC_FAIL             1u
#define NUM8LORA_OP_ERR_PROTOCOL             2u
#define NUM8LORA_OP_ERR_MSG_TYPE             3u
#define NUM8LORA_OP_ERR_RECEIVER             4u
#define NUM8LORA_OP_ERR_STREAM               5u
#define NUM8LORA_OP_ERR_SEQUENCE_GAP         6u
#define NUM8LORA_OP_ERR_INVALID_VALUE        7u
#define NUM8LORA_OP_ERR_APPLY_FAILED         8u

typedef struct num8lora_op_common_header_s
{
    uint8_t protocol_version;
    uint8_t msg_type;
    uint16_t sender_id;
    uint16_t receiver_id;
    uint16_t seq;
} num8lora_op_common_header_t;

typedef struct num8lora_op_beacon_payload_s
{
    uint32_t stream_id;
    uint32_t latest_op_id;
} num8lora_op_beacon_payload_t;

typedef struct num8lora_op_request_payload_s
{
    uint32_t stream_id;
    uint32_t next_needed_op_id;
} num8lora_op_request_payload_t;

typedef struct num8lora_op_data_payload_s
{
    uint32_t stream_id;
    uint32_t op_id;
    uint8_t op_type;
    uint8_t reserved0;
    uint16_t reserved1;
    uint32_t value;
} num8lora_op_data_payload_t;

typedef struct num8lora_op_ack_payload_s
{
    uint32_t stream_id;
    uint32_t ack_op_id;
} num8lora_op_ack_payload_t;

typedef struct num8lora_op_nack_payload_s
{
    uint32_t stream_id;
    uint16_t error_code;
    uint16_t detail;
    uint32_t expected_next_op_id;
} num8lora_op_nack_payload_t;

uint16_t num8lora_op_crc16_ccitt_false(const void* data, uint32_t len);

int num8lora_op_encode_beacon(uint8_t* out_buf, uint32_t out_cap, uint16_t sender_id, uint16_t seq, const num8lora_op_beacon_payload_t* p, uint32_t* out_len);
int num8lora_op_encode_request(uint8_t* out_buf, uint32_t out_cap, uint16_t sender_id, uint16_t receiver_id, uint16_t seq, const num8lora_op_request_payload_t* p, uint32_t* out_len);
int num8lora_op_encode_data(uint8_t* out_buf, uint32_t out_cap, uint16_t sender_id, uint16_t receiver_id, uint16_t seq, const num8lora_op_data_payload_t* p, uint32_t* out_len);
int num8lora_op_encode_ack(uint8_t* out_buf, uint32_t out_cap, uint16_t sender_id, uint16_t receiver_id, uint16_t seq, const num8lora_op_ack_payload_t* p, uint32_t* out_len);
int num8lora_op_encode_nack(uint8_t* out_buf, uint32_t out_cap, uint16_t sender_id, uint16_t receiver_id, uint16_t seq, const num8lora_op_nack_payload_t* p, uint32_t* out_len);

int num8lora_op_decode_common_header(const uint8_t* buf, uint32_t len, num8lora_op_common_header_t* out_hdr);
int num8lora_op_validate_frame_crc(const uint8_t* buf, uint32_t len);

int num8lora_op_decode_beacon(const uint8_t* buf, uint32_t len, num8lora_op_common_header_t* out_hdr, num8lora_op_beacon_payload_t* out_p);
int num8lora_op_decode_request(const uint8_t* buf, uint32_t len, num8lora_op_common_header_t* out_hdr, num8lora_op_request_payload_t* out_p);
int num8lora_op_decode_data(const uint8_t* buf, uint32_t len, num8lora_op_common_header_t* out_hdr, num8lora_op_data_payload_t* out_p);
int num8lora_op_decode_ack(const uint8_t* buf, uint32_t len, num8lora_op_common_header_t* out_hdr, num8lora_op_ack_payload_t* out_p);
int num8lora_op_decode_nack(const uint8_t* buf, uint32_t len, num8lora_op_common_header_t* out_hdr, num8lora_op_nack_payload_t* out_p);

#ifdef __cplusplus
}
#endif

#endif
