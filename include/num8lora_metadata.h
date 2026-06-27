#ifndef NUM8LORA_METADATA_H
#define NUM8LORA_METADATA_H

#include <stdint.h>

#if defined(_WIN32) && defined(NUM8LORA_BUILD_SHARED)
#define NUM8LORA_METADATA_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#define NUM8LORA_METADATA_API __attribute__((visibility("default")))
#else
#define NUM8LORA_METADATA_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef int num8lora_metadata_status_t;

#define NUM8LORA_METADATA_STATUS_FAILURE 0
#define NUM8LORA_METADATA_STATUS_SUCCESS 1

enum
{
    NUM8LORA_METADATA_ERR_NONE = 0,
    NUM8LORA_METADATA_ERR_MAGIC = 1,
    NUM8LORA_METADATA_ERR_VERSION = 2,
    NUM8LORA_METADATA_ERR_RESERVED = 3,
    NUM8LORA_METADATA_ERR_CRC = 4,
    NUM8LORA_METADATA_ERR_LENGTH = 5,
    NUM8LORA_METADATA_ERR_IO = 6,
    NUM8LORA_METADATA_ERR_INTERNAL = 7
};

enum
{
    NUM8LORA_METADATA_FORMAT_VERSION = 1u
};

typedef struct num8lora_metadata_record_s
{
    uint32_t format_version;
    uint32_t stream_id;
    uint32_t stream_epoch;
    uint32_t last_applied_op_id;
    uint32_t reserved0;
} num8lora_metadata_record_t;

NUM8LORA_METADATA_API uint32_t num8lora_metadata_record_size(void);

NUM8LORA_METADATA_API num8lora_metadata_status_t num8lora_metadata_encode_record(
    const num8lora_metadata_record_t* record,
    uint8_t* out_buf,
    uint32_t out_cap,
    uint32_t* out_len,
    uint16_t* out_error_code);

NUM8LORA_METADATA_API num8lora_metadata_status_t num8lora_metadata_decode_record(
    const uint8_t* buf,
    uint32_t len,
    num8lora_metadata_record_t* out_record,
    uint16_t* out_error_code);

NUM8LORA_METADATA_API num8lora_metadata_status_t num8lora_metadata_record_matches_stream(
    const num8lora_metadata_record_t* record,
    uint32_t stream_id,
    uint32_t stream_epoch,
    uint8_t* out_matches);

#ifdef NUM8LORA_BUILD_HOSTED_METADATA
NUM8LORA_METADATA_API num8lora_metadata_status_t num8lora_metadata_save_file(
    const char* path,
    const num8lora_metadata_record_t* record,
    uint16_t* out_error_code);

NUM8LORA_METADATA_API num8lora_metadata_status_t num8lora_metadata_load_file(
    const char* path,
    num8lora_metadata_record_t* out_record,
    uint16_t* out_error_code);
#endif

#ifdef __cplusplus
}
#endif

#endif
