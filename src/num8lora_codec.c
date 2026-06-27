#include "num8lora_codec.h"

uint16_t num8lora_codec_crc16_ccitt_false(const void* data, uint32_t len)
{
    const uint8_t* p = (const uint8_t*)data;
    uint16_t crc = 0xFFFFu;
    uint32_t i;

    if (p == NULL && len != 0u)
    {
        return 0u;
    }

    for (i = 0u; i < len; ++i)
    {
        int b;

        crc ^= (uint16_t)((uint16_t)p[i] << 8);
        for (b = 0; b < 8; ++b)
        {
            if ((crc & 0x8000u) != 0u)
            {
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            }
            else
            {
                crc = (uint16_t)(crc << 1);
            }
        }
    }

    return crc;
}

uint16_t num8lora_codec_read_u16_le(const uint8_t* p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

uint32_t num8lora_codec_read_u32_le(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

void num8lora_codec_write_u16_le(uint8_t* p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

void num8lora_codec_write_u32_le(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

int num8lora_codec_add_u32(uint32_t a, uint32_t b, uint32_t* out)
{
    uint64_t sum = (uint64_t)a + (uint64_t)b;
    if (sum > 0xFFFFFFFFu || out == NULL)
    {
        return 0;
    }
    *out = (uint32_t)sum;
    return 1;
}

int num8lora_codec_mul_u32(uint32_t a, uint32_t b, uint32_t* out)
{
    uint64_t prod = (uint64_t)a * (uint64_t)b;
    if (prod > 0xFFFFFFFFu || out == NULL)
    {
        return 0;
    }
    *out = (uint32_t)prod;
    return 1;
}
