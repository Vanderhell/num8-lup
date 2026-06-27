#include "num8lora_metadata.h"
#include "num8lora_codec.h"

#include <stddef.h>
#include <stdio.h>

#if defined(_WIN32)
#include <windows.h>
#endif

static int build_temp_path(char* out, size_t out_cap, const char* path)
{
    size_t path_len = 0u;
    size_t i;

    if (out == NULL || path == NULL)
    {
        return 0;
    }

    while (path[path_len] != '\0')
    {
        ++path_len;
    }

    if (path_len + 4u + 1u > out_cap)
    {
        return 0;
    }

    for (i = 0u; i < path_len; ++i)
    {
        out[i] = path[i];
    }
    out[path_len + 0u] = '.';
    out[path_len + 1u] = 't';
    out[path_len + 2u] = 'm';
    out[path_len + 3u] = 'p';
    out[path_len + 4u] = '\0';
    return 1;
}

static int write_record_file(FILE* f, const uint8_t* buf, uint32_t len)
{
    int ok = 0;

    if (f == NULL)
    {
        return 0;
    }

    ok = (fwrite(buf, 1u, len, f) == len);
    if (ok)
    {
        ok = (fflush(f) == 0);
    }
    if (fclose(f) != 0)
    {
        ok = 0;
    }
    return ok;
}

static int replace_file_atomically(const char* temp_path, const char* final_path)
{
#if defined(_WIN32)
    return MoveFileExA(temp_path, final_path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return rename(temp_path, final_path) == 0;
#endif
}

num8lora_metadata_status_t num8lora_metadata_save_file(
    const char* path,
    const num8lora_metadata_record_t* record,
    uint16_t* out_error_code)
{
    uint8_t buf[26u];
    uint32_t len = 0u;
    char temp_path[1024u];
    FILE* f = NULL;

    if (out_error_code != NULL)
    {
        *out_error_code = NUM8LORA_METADATA_ERR_NONE;
    }
    if (path == NULL || record == NULL)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_METADATA_ERR_INTERNAL;
        }
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }

    if (!num8lora_metadata_encode_record(record, buf, sizeof(buf), &len, out_error_code))
    {
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }

    if (!build_temp_path(temp_path, sizeof(temp_path), path))
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_METADATA_ERR_IO;
        }
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }

#if defined(_WIN32)
    if (fopen_s(&f, temp_path, "wb") != 0)
    {
        f = NULL;
    }
#else
    f = fopen(temp_path, "wb");
#endif
    if (f == NULL)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_METADATA_ERR_IO;
        }
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }

    if (!write_record_file(f, buf, len))
    {
        remove(temp_path);
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_METADATA_ERR_IO;
        }
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }

    if (!replace_file_atomically(temp_path, path))
    {
        remove(temp_path);
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_METADATA_ERR_IO;
        }
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }

    return NUM8LORA_METADATA_STATUS_SUCCESS;
}

num8lora_metadata_status_t num8lora_metadata_load_file(
    const char* path,
    num8lora_metadata_record_t* out_record,
    uint16_t* out_error_code)
{
    uint8_t buf[26u];
    FILE* f;
    long extra;

    if (out_record != NULL)
    {
        num8lora_codec_zero_bytes(out_record, (uint32_t)sizeof(*out_record));
    }
    if (out_error_code != NULL)
    {
        *out_error_code = NUM8LORA_METADATA_ERR_NONE;
    }
    if (path == NULL || out_record == NULL)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_METADATA_ERR_INTERNAL;
        }
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }

#if defined(_WIN32)
    if (fopen_s(&f, path, "rb") != 0)
    {
        f = NULL;
    }
#else
    f = fopen(path, "rb");
#endif
    if (f == NULL)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_METADATA_ERR_IO;
        }
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }

    if (fread(buf, 1u, sizeof(buf), f) != sizeof(buf))
    {
        fclose(f);
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_METADATA_ERR_LENGTH;
        }
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }

    extra = fgetc(f);
    if (extra != EOF)
    {
        fclose(f);
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_METADATA_ERR_LENGTH;
        }
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }

    if (fclose(f) != 0)
    {
        if (out_error_code != NULL)
        {
            *out_error_code = NUM8LORA_METADATA_ERR_IO;
        }
        return NUM8LORA_METADATA_STATUS_FAILURE;
    }

    return num8lora_metadata_decode_record(buf, sizeof(buf), out_record, out_error_code);
}
