#ifndef NUM8LORA_CODEC_H
#define NUM8LORA_CODEC_H

#include <stdint.h>

uint16_t num8lora_codec_crc16_ccitt_false(const void* data, uint32_t len);
uint16_t num8lora_codec_read_u16_le(const uint8_t* p);
uint32_t num8lora_codec_read_u32_le(const uint8_t* p);
void num8lora_codec_write_u16_le(uint8_t* p, uint16_t v);
void num8lora_codec_write_u32_le(uint8_t* p, uint32_t v);
int num8lora_codec_add_u32(uint32_t a, uint32_t b, uint32_t* out);
int num8lora_codec_mul_u32(uint32_t a, uint32_t b, uint32_t* out);
void num8lora_codec_zero_bytes(void* p, uint32_t len);

#endif
