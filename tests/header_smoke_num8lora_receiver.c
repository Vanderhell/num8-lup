#include "num8lora_receiver.h"

int main(void)
{
    num8lora_receiver_t ctx;
    num8lora_receiver_init(&ctx, 1u, 2u, 0u, 0u);
    return 0;
}
