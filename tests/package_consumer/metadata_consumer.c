#include "num8lora_metadata.h"

#include <stdio.h>

int main(void)
{
    num8lora_metadata_record_t in = { NUM8LORA_METADATA_FORMAT_VERSION, 100u, 7u, 33u, 0u };
    num8lora_metadata_record_t out;
    uint8_t buf[64];
    uint32_t len = 0u;
    uint16_t err = 0u;

    if (!num8lora_metadata_encode_record(&in, buf, sizeof(buf), &len, &err))
    {
        return 1;
    }
    if (!num8lora_metadata_decode_record(buf, len, &out, &err))
    {
        return 2;
    }
    if (out.stream_id != in.stream_id || out.last_applied_op_id != in.last_applied_op_id)
    {
        return 3;
    }
    puts("metadata consumer ok");
    return 0;
}
