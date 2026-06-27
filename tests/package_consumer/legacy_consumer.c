#include "num8lora.h"

#include <stdio.h>

int main(void)
{
    uint8_t buf[64];
    uint32_t len = 0u;
    num8lora_beacon_payload_t in = { 7u, 1u, 2u, 1u, 1u, 24u, 0u };
    num8lora_common_header_t hdr;
    num8lora_beacon_payload_t out;

    if (!num8lora_encode_beacon(buf, sizeof(buf), 11u, 3u, &in, &len))
    {
        return 1;
    }
    if (!num8lora_decode_beacon(buf, len, &hdr, &out))
    {
        return 2;
    }
    if (hdr.sender_id != 11u || out.current_update_id != 7u || out.update_payload_size != 24u)
    {
        return 3;
    }
    puts("legacy consumer ok");
    return 0;
}
