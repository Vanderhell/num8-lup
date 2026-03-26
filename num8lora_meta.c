#include "num8lora_op.h"

#include <stdio.h>
#include <string.h>

static void st16(uint8_t* p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static void st32(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static uint16_t ld16(const uint8_t* p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t ld32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int num8lora_save_receiver_meta(const char* path, uint32_t dataset_version, uint32_t last_applied_update_id)
{
    uint8_t rec[18];
    uint16_t crc;
    FILE* f;

    if (path == NULL)
    {
        return 0;
    }

    rec[0] = 'N';
    rec[1] = '8';
    rec[2] = 'L';
    rec[3] = 'M';
    st16(&rec[4], 1u);
    st16(&rec[6], 0u);
    st32(&rec[8], dataset_version);
    st32(&rec[12], last_applied_update_id);
    crc = num8lora_op_crc16_ccitt_false(rec, 16u);
    st16(&rec[16], crc);

    f = fopen(path, "wb");
    if (f == NULL)
    {
        return 0;
    }
    if (fwrite(rec, 1, sizeof(rec), f) != sizeof(rec))
    {
        fclose(f);
        return 0;
    }
    fclose(f);
    return 1;
}

int num8lora_load_receiver_meta(const char* path, uint32_t* out_dataset_version, uint32_t* out_last_applied_update_id)
{
    uint8_t rec[18];
    uint16_t crc;
    FILE* f;

    if (path == NULL || out_dataset_version == NULL || out_last_applied_update_id == NULL)
    {
        return 0;
    }

    f = fopen(path, "rb");
    if (f == NULL)
    {
        return 0;
    }
    if (fread(rec, 1, sizeof(rec), f) != sizeof(rec))
    {
        fclose(f);
        return 0;
    }
    fclose(f);

    if (rec[0] != 'N' || rec[1] != '8' || rec[2] != 'L' || rec[3] != 'M')
    {
        return 0;
    }
    if (ld16(&rec[4]) != 1u)
    {
        return 0;
    }
    crc = num8lora_op_crc16_ccitt_false(rec, 16u);
    if (crc != ld16(&rec[16]))
    {
        return 0;
    }

    *out_dataset_version = ld32(&rec[8]);
    *out_last_applied_update_id = ld32(&rec[12]);
    return 1;
}
