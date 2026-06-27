#include <stddef.h>
#include "num8lora.h"

int header_smoke_mtu_helper(void);

int header_smoke_mtu_helper(void)
{
    return (num8lora_crc16_ccitt_false(NULL, 0u) == 0xFFFFu) ? 0 : 1;
}
