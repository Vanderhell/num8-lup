#ifndef NUM8LORA_H
#define NUM8LORA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NUM8LORA_PROTOCOL_VERSION 1u

#define NUM8LORA_MSG_BEACON         0x01u
#define NUM8LORA_MSG_UPDATE_REQUEST 0x02u
#define NUM8LORA_MSG_UPDATE_DATA    0x03u
#define NUM8LORA_MSG_ACK            0x04u
#define NUM8LORA_MSG_NACK           0x05u

#define NUM8LORA_ANY_RECEIVER_ID 0xFFFFu

enum
{
    NUM8LORA_ERR_NONE = 0,
    NUM8LORA_ERR_CRC_FAIL = 1,
    NUM8LORA_ERR_PROTOCOL_VERSION_UNSUPPORTED = 2,
    NUM8LORA_ERR_INVALID_MESSAGE_TYPE = 3,
    NUM8LORA_ERR_INVALID_RECEIVER_ID = 4,
    NUM8LORA_ERR_UPDATE_ID_UNKNOWN = 5,
    NUM8LORA_ERR_DATASET_VERSION_MISMATCH = 6,
    NUM8LORA_ERR_INVALID_NUMBER = 7,
    NUM8LORA_ERR_DUPLICATE_IN_BATCH = 8,
    NUM8LORA_ERR_NUMBER_IN_BOTH = 9,
    NUM8LORA_ERR_APPLY_FAILED = 10,
    NUM8LORA_ERR_BUSY = 11,
    NUM8LORA_ERR_INTERNAL = 12
};

typedef enum num8lora_sender_state_e
{
    NUM8LORA_SENDER_IDLE = 0,
    NUM8LORA_SENDER_WAIT_REQUEST = 1,
    NUM8LORA_SENDER_WAIT_ACK = 2
} num8lora_sender_state_t;

typedef enum num8lora_receiver_state_e
{
    NUM8LORA_RECEIVER_SCAN = 0,
    NUM8LORA_RECEIVER_WAIT_UPDATE = 1
} num8lora_receiver_state_t;

typedef enum num8lora_sender_ack_result_e
{
    NUM8LORA_ACKRES_NONE = 0,
    NUM8LORA_ACKRES_APPLIED = 1,
    NUM8LORA_ACKRES_NACKED = 2
} num8lora_sender_ack_result_t;

typedef struct num8lora_common_header_s
{
    uint8_t protocol_version;
    uint8_t msg_type;
    uint16_t sender_id;
    uint16_t receiver_id;
    uint16_t seq;
} num8lora_common_header_t;

typedef struct num8lora_beacon_payload_s
{
    uint32_t current_update_id;
    uint32_t dataset_version_from;
    uint32_t dataset_version_to;
    uint8_t remove_count;
    uint8_t add_count;
    uint16_t update_payload_size;
    uint16_t update_payload_crc16;
} num8lora_beacon_payload_t;

typedef struct num8lora_update_request_payload_s
{
    uint32_t requested_update_id;
    uint32_t receiver_dataset_ver;
} num8lora_update_request_payload_t;

typedef struct num8lora_update_header_s
{
    uint32_t update_id;
    uint32_t dataset_version_from;
    uint32_t dataset_version_to;
    uint8_t remove_count;
    uint8_t add_count;
    uint16_t reserved0;
} num8lora_update_header_t;

typedef struct num8lora_ack_payload_s
{
    uint32_t ack_update_id;
    uint32_t receiver_dataset_ver;
} num8lora_ack_payload_t;

typedef struct num8lora_nack_payload_s
{
    uint32_t nack_update_id;
    uint16_t error_code;
    uint16_t detail;
} num8lora_nack_payload_t;

typedef struct num8lora_sender_ctx_s
{
    num8lora_sender_state_t state;
    uint16_t sender_id;
    uint16_t target_receiver_id;
    uint16_t seq;
    uint8_t max_retries;
    uint8_t retries;

    num8lora_update_header_t update_hdr;
    const uint32_t* remove_numbers;
    const uint32_t* add_numbers;
    uint16_t update_payload_crc16;

    uint16_t last_nack_error;
} num8lora_sender_ctx_t;

typedef struct num8lora_receiver_ctx_s
{
    num8lora_receiver_state_t state;
    uint16_t receiver_id;
    uint16_t expected_sender_id;
    uint16_t seq;
    uint8_t max_retries;
    uint8_t retries;

    uint32_t local_dataset_version;
    uint32_t last_applied_update_id;

    uint32_t pending_update_id;
    uint32_t pending_dataset_version_from;
    uint32_t pending_dataset_version_to;
    uint8_t pending_remove_count;
    uint8_t pending_add_count;
    uint16_t pending_payload_size;
    uint16_t pending_payload_crc16;
} num8lora_receiver_ctx_t;

uint16_t num8lora_crc16_ccitt_false(const void* data, uint32_t len);

int num8lora_encode_beacon(
    uint8_t* out_buf,
    uint32_t out_cap,
    uint16_t sender_id,
    uint16_t seq,
    const num8lora_beacon_payload_t* payload,
    uint32_t* out_len);

int num8lora_encode_update_request(
    uint8_t* out_buf,
    uint32_t out_cap,
    uint16_t sender_id,
    uint16_t receiver_id,
    uint16_t seq,
    const num8lora_update_request_payload_t* payload,
    uint32_t* out_len);

int num8lora_encode_update_data(
    uint8_t* out_buf,
    uint32_t out_cap,
    uint16_t sender_id,
    uint16_t receiver_id,
    uint16_t seq,
    const num8lora_update_header_t* header,
    const uint32_t* remove_numbers,
    const uint32_t* add_numbers,
    uint32_t* out_len);

int num8lora_encode_ack(
    uint8_t* out_buf,
    uint32_t out_cap,
    uint16_t sender_id,
    uint16_t receiver_id,
    uint16_t seq,
    const num8lora_ack_payload_t* payload,
    uint32_t* out_len);

int num8lora_encode_nack(
    uint8_t* out_buf,
    uint32_t out_cap,
    uint16_t sender_id,
    uint16_t receiver_id,
    uint16_t seq,
    const num8lora_nack_payload_t* payload,
    uint32_t* out_len);

int num8lora_decode_common_header(
    const uint8_t* buf,
    uint32_t len,
    num8lora_common_header_t* out_hdr);

int num8lora_validate_frame_crc(
    const uint8_t* buf,
    uint32_t len);

int num8lora_decode_beacon(
    const uint8_t* buf,
    uint32_t len,
    num8lora_common_header_t* out_hdr,
    num8lora_beacon_payload_t* out_payload);

int num8lora_decode_update_request(
    const uint8_t* buf,
    uint32_t len,
    num8lora_common_header_t* out_hdr,
    num8lora_update_request_payload_t* out_payload);

int num8lora_decode_update_data_header(
    const uint8_t* buf,
    uint32_t len,
    num8lora_common_header_t* out_hdr,
    num8lora_update_header_t* out_upd_hdr,
    const uint8_t** out_remove_ptr,
    const uint8_t** out_add_ptr);

int num8lora_decode_ack(
    const uint8_t* buf,
    uint32_t len,
    num8lora_common_header_t* out_hdr,
    num8lora_ack_payload_t* out_payload);

int num8lora_decode_nack(
    const uint8_t* buf,
    uint32_t len,
    num8lora_common_header_t* out_hdr,
    num8lora_nack_payload_t* out_payload);

int num8lora_validate_update_numbers(
    const num8lora_update_header_t* hdr,
    const uint32_t* remove_numbers,
    const uint32_t* add_numbers,
    uint16_t* out_error_code);

int num8lora_decode_update_numbers(
    const num8lora_update_header_t* hdr,
    const uint8_t* remove_ptr,
    const uint8_t* add_ptr,
    uint32_t* out_remove_numbers,
    uint32_t* out_add_numbers);

int num8lora_compute_update_payload_crc16(
    const num8lora_update_header_t* hdr,
    const uint32_t* remove_numbers,
    const uint32_t* add_numbers,
    uint16_t* out_crc16);

int num8lora_classify_frame(
    const uint8_t* buf,
    uint32_t len,
    uint16_t expected_receiver_id,
    uint8_t expected_msg_type,
    num8lora_common_header_t* out_hdr,
    uint16_t* out_error_code);

void num8lora_sender_init(num8lora_sender_ctx_t* ctx, uint16_t sender_id, uint8_t max_retries);
int num8lora_sender_set_update(
    num8lora_sender_ctx_t* ctx,
    uint16_t target_receiver_id,
    const num8lora_update_header_t* hdr,
    const uint32_t* remove_numbers,
    const uint32_t* add_numbers,
    uint16_t* out_error_code);
int num8lora_sender_build_beacon(num8lora_sender_ctx_t* ctx, uint8_t* out_buf, uint32_t out_cap, uint32_t* out_len);
int num8lora_sender_handle_update_request(
    num8lora_sender_ctx_t* ctx,
    const uint8_t* req_buf,
    uint32_t req_len,
    uint8_t* out_response_buf,
    uint32_t out_cap,
    uint32_t* out_len);
int num8lora_sender_handle_ack_or_nack(
    num8lora_sender_ctx_t* ctx,
    const uint8_t* in_buf,
    uint32_t in_len,
    num8lora_sender_ack_result_t* out_result,
    uint16_t* out_nack_error_code);
int num8lora_sender_on_ack_timeout(
    num8lora_sender_ctx_t* ctx,
    uint8_t* out_update_buf,
    uint32_t out_cap,
    uint32_t* out_len);

void num8lora_receiver_init(
    num8lora_receiver_ctx_t* ctx,
    uint16_t receiver_id,
    uint16_t expected_sender_id,
    uint8_t max_retries,
    uint32_t local_dataset_version,
    uint32_t last_applied_update_id);
int num8lora_receiver_handle_beacon(
    num8lora_receiver_ctx_t* ctx,
    const uint8_t* beacon_buf,
    uint32_t beacon_len,
    uint8_t* out_request_buf,
    uint32_t out_cap,
    uint32_t* out_len);
int num8lora_receiver_on_update_timeout(
    num8lora_receiver_ctx_t* ctx,
    uint8_t* out_request_buf,
    uint32_t out_cap,
    uint32_t* out_len);
int num8lora_receiver_validate_update_data(
    num8lora_receiver_ctx_t* ctx,
    const uint8_t* update_buf,
    uint32_t update_len,
    num8lora_update_header_t* out_hdr,
    uint32_t* out_remove_numbers,
    uint32_t* out_add_numbers,
    uint16_t* out_error_code);
int num8lora_receiver_encode_ack(
    num8lora_receiver_ctx_t* ctx,
    uint16_t sender_id,
    uint32_t ack_update_id,
    uint8_t* out_ack_buf,
    uint32_t out_cap,
    uint32_t* out_len);
int num8lora_receiver_encode_nack(
    num8lora_receiver_ctx_t* ctx,
    uint16_t sender_id,
    uint32_t nack_update_id,
    uint16_t error_code,
    uint16_t detail,
    uint8_t* out_nack_buf,
    uint32_t out_cap,
    uint32_t* out_len);

int num8lora_save_receiver_meta(const char* path, uint32_t dataset_version, uint32_t last_applied_update_id);
int num8lora_load_receiver_meta(const char* path, uint32_t* out_dataset_version, uint32_t* out_last_applied_update_id);

#ifdef NUM8LORA_ENABLE_NUM8
#include "num8.h"
int num8lora_apply_update_to_num8(
    num8_engine_t* engine,
    uint32_t local_dataset_version,
    uint32_t last_applied_update_id,
    const num8lora_update_header_t* hdr,
    const uint32_t* remove_numbers,
    const uint32_t* add_numbers,
    uint32_t* out_new_dataset_version,
    uint32_t* out_new_last_applied_update_id,
    uint16_t* out_error_code);
int num8lora_receiver_apply_update_data_to_num8(
    num8lora_receiver_ctx_t* ctx,
    num8_engine_t* engine,
    const uint8_t* update_buf,
    uint32_t update_len,
    uint8_t* out_response_buf,
    uint32_t out_cap,
    uint32_t* out_response_len,
    uint16_t* out_error_code);
#endif

#ifdef __cplusplus
}
#endif

#endif
