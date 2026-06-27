#include "num8lora_metadata.h"

#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d\n", __LINE__); return 1; } } while (0)

static int record_equal(const num8lora_metadata_record_t* a, const num8lora_metadata_record_t* b)
{
    return a->format_version == b->format_version
        && a->stream_id == b->stream_id
        && a->stream_epoch == b->stream_epoch
        && a->last_applied_op_id == b->last_applied_op_id
        && a->reserved0 == b->reserved0;
}

static FILE* open_file_write(const char* path)
{
    FILE* f = NULL;
#if defined(_WIN32)
    if (fopen_s(&f, path, "wb") != 0)
    {
        f = NULL;
    }
#else
    f = fopen(path, "wb");
#endif
    return f;
}

static int test_record_codec(void)
{
    num8lora_metadata_record_t rec = { NUM8LORA_METADATA_FORMAT_VERSION, 77u, 12u, 34u, 0u };
    num8lora_metadata_record_t out;
    uint8_t buf[64];
    uint32_t len = 0u;
    uint16_t err = 0u;
    uint8_t matches = 0u;

    CHECK(num8lora_metadata_encode_record(&rec, buf, sizeof(buf), &len, &err));
    CHECK(len == num8lora_metadata_record_size());
    CHECK(num8lora_metadata_decode_record(buf, len, &out, &err));
    CHECK(record_equal(&rec, &out));
    CHECK(num8lora_metadata_record_matches_stream(&out, 77u, 12u, &matches));
    CHECK(matches == 1u);
    CHECK(num8lora_metadata_record_matches_stream(&out, 99u, 12u, &matches));
    CHECK(matches == 0u);

    buf[0] = 'X';
    CHECK(!num8lora_metadata_decode_record(buf, len, &out, &err));
    CHECK(err == NUM8LORA_METADATA_ERR_MAGIC);

    CHECK(num8lora_metadata_encode_record(&rec, buf, sizeof(buf), &len, &err));
    buf[4] = 2u;
    CHECK(!num8lora_metadata_decode_record(buf, len, &out, &err));
    CHECK(err == NUM8LORA_METADATA_ERR_VERSION);

    CHECK(num8lora_metadata_encode_record(&rec, buf, sizeof(buf), &len, &err));
    buf[24] ^= 0x01u;
    CHECK(!num8lora_metadata_decode_record(buf, len, &out, &err));
    CHECK(err == NUM8LORA_METADATA_ERR_CRC);

    CHECK(num8lora_metadata_encode_record(&rec, buf, sizeof(buf), &len, &err));
    buf[8] = 1u;
    CHECK(!num8lora_metadata_decode_record(buf, len, &out, &err));
    CHECK(err == NUM8LORA_METADATA_ERR_RESERVED);

    CHECK(!num8lora_metadata_decode_record(buf, len - 1u, &out, &err));
    CHECK(err == NUM8LORA_METADATA_ERR_LENGTH);
    CHECK(!num8lora_metadata_decode_record(buf, len + 1u, &out, &err));
    CHECK(err == NUM8LORA_METADATA_ERR_LENGTH);

    return 0;
}

#if NUM8LORA_BUILD_HOSTED_METADATA
static int test_file_io(void)
{
    const char* valid_path = "num8lora_metadata_test.meta";
    const char* trailing_path = "num8lora_metadata_test.trailing";
    const char* truncated_path = "num8lora_metadata_test.trunc";
    const char* temp_dir = "num8lora_metadata_test.meta.tmp";
    num8lora_metadata_record_t rec = { NUM8LORA_METADATA_FORMAT_VERSION, 88u, 44u, 55u, 0u };
    num8lora_metadata_record_t changed = { NUM8LORA_METADATA_FORMAT_VERSION, 88u, 44u, 999u, 0u };
    num8lora_metadata_record_t out;
    uint8_t buf[64];
    uint32_t len = 0u;
    uint16_t err = 0u;
    FILE* f;
    int ch;

    CHECK(num8lora_metadata_save_file(valid_path, &rec, &err));
    CHECK(num8lora_metadata_load_file(valid_path, &out, &err));
    CHECK(record_equal(&rec, &out));

#if defined(_WIN32)
    _rmdir(temp_dir);
    CHECK(_mkdir(temp_dir) == 0);
#else
    rmdir(temp_dir);
    CHECK(mkdir(temp_dir, 0777) == 0);
#endif
    CHECK(!num8lora_metadata_save_file(valid_path, &changed, &err));
    CHECK(num8lora_metadata_load_file(valid_path, &out, &err));
    CHECK(record_equal(&rec, &out));

    CHECK(num8lora_metadata_encode_record(&rec, buf, sizeof(buf), &len, &err));

    f = open_file_write(trailing_path);
    CHECK(f != NULL);
    CHECK(fwrite(buf, 1u, len, f) == len);
    ch = fputc('X', f);
    CHECK(ch != EOF);
    CHECK(fclose(f) == 0);
    CHECK(!num8lora_metadata_load_file(trailing_path, &out, &err));
    CHECK(err == NUM8LORA_METADATA_ERR_LENGTH);

    f = open_file_write(truncated_path);
    CHECK(f != NULL);
    CHECK(fwrite(buf, 1u, len - 1u, f) == len - 1u);
    CHECK(fclose(f) == 0);
    CHECK(!num8lora_metadata_load_file(truncated_path, &out, &err));
    CHECK(err == NUM8LORA_METADATA_ERR_LENGTH);

    remove(valid_path);
#if defined(_WIN32)
    _rmdir(temp_dir);
#else
    rmdir(temp_dir);
#endif
    remove(trailing_path);
    remove(truncated_path);
    return 0;
}
#endif

int main(void)
{
    CHECK(test_record_codec() == 0);
#if NUM8LORA_BUILD_HOSTED_METADATA
    CHECK(test_file_io() == 0);
#endif
    printf("num8lora_metadata_test OK\n");
    return 0;
}
