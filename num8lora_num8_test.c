#include "num8lora.h"
#include "num8.h"

#include <stdio.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d\n", __LINE__); return 1; } } while (0)

int main(void)
{
    const char* f = "lora_num8_apply_tmp.num8";
    num8_engine_t e;
    num8_status_t st;
    num8lora_update_header_t h;
    uint32_t rem[2] = {100u, 200u};
    uint32_t add[3] = {300u, 400u, 500u};
    uint32_t new_ver = 0;
    uint32_t new_upd = 0;
    uint16_t err = 0;
    int ex = 0;

    st = num8_create(f, &e);
    CHECK(st == NUM8_STATUS_OK);

    CHECK(num8_add_u32(&e, 100u) == NUM8_STATUS_ADDED);
    CHECK(num8_add_u32(&e, 200u) == NUM8_STATUS_ADDED);
    CHECK(num8_flush(&e) == NUM8_STATUS_OK);

    h.update_id = 11u;
    h.dataset_version_from = 7u;
    h.dataset_version_to = 8u;
    h.remove_count = 2u;
    h.add_count = 3u;
    h.reserved0 = 0u;

    CHECK(num8lora_apply_update_to_num8(&e, 7u, 10u, &h, rem, add, &new_ver, &new_upd, &err));
    CHECK(err == NUM8LORA_ERR_NONE);
    CHECK(new_ver == 8u);
    CHECK(new_upd == 11u);

    CHECK(num8_exists_u32(&e, 100u, &ex) == NUM8_STATUS_OK && ex == 0);
    CHECK(num8_exists_u32(&e, 200u, &ex) == NUM8_STATUS_OK && ex == 0);
    CHECK(num8_exists_u32(&e, 300u, &ex) == NUM8_STATUS_OK && ex == 1);
    CHECK(num8_exists_u32(&e, 400u, &ex) == NUM8_STATUS_OK && ex == 1);
    CHECK(num8_exists_u32(&e, 500u, &ex) == NUM8_STATUS_OK && ex == 1);

    CHECK(num8_close(&e) == NUM8_STATUS_OK);
    remove(f);

    printf("num8lora_num8_test OK\n");
    return 0;
}