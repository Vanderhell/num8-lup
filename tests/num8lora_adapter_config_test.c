#include "num8lora.h"

#include <stdio.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d\n", __LINE__); return 1; } } while (0)

int main(void)
{
#if NUM8LORA_BUILD_NUM8_ADAPTER
    num8_engine_t e;
    num8lora_update_header_t h;
    uint32_t rem[1] = { 100u };
    uint32_t add[1] = { 200u };
    uint32_t new_ver = 0u;
    uint32_t new_last = 0u;
    uint16_t err = 0u;

    CHECK(num8_create("num8lora_adapter_config_test.num8", &e) == NUM8_STATUS_OK);
    CHECK(num8_add_u32(&e, 100u) == NUM8_STATUS_ADDED);
    CHECK(num8_flush(&e) == NUM8_STATUS_OK);

    h.update_id = 9u;
    h.dataset_version_from = 7u;
    h.dataset_version_to = 8u;
    h.remove_count = 1u;
    h.add_count = 1u;
    h.reserved0 = 0u;

    CHECK(num8lora_apply_update_to_num8(&e, 7u, 4u, &h, rem, add, &new_ver, &new_last, &err));
    CHECK(err == NUM8LORA_ERR_NONE);
    CHECK(new_ver == 8u);
    CHECK(new_last == 9u);
    CHECK(num8_close(&e) == NUM8_STATUS_OK);
    remove("num8lora_adapter_config_test.num8");
#else
#ifdef NUM8LORA_ENABLE_NUM8
#error "adapter config test expected NUM8LORA_ENABLE_NUM8 to be undefined in a non-adapter build"
#endif
#endif

    printf("num8lora_adapter_config_test OK\n");
    return 0;
}
