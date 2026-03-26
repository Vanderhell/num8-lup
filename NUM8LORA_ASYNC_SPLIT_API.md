# NUM8LORA Sender/Receiver Async DLL API

Toto je nov˝ split model pre 1 vysielaË a viac prijÌmaËov.

## DLL s˙bory

- `build/Release/num8lora_sender.dll`
- `build/Release/num8lora_receiver.dll`

## 1. Sender strana

Header: `num8lora_sender.h`

HlavnÈ API:
- `num8lora_sender_init(...)`
- `num8lora_sender_register_receiver(...)`
- `num8lora_sender_enqueue_add(...)`
- `num8lora_sender_enqueue_remove(...)`
- `num8lora_sender_enqueue_lists(...)`
- `num8lora_sender_build_beacon(...)`
- `num8lora_sender_poll_tx(...)` (async/non-blocking scheduler)
- `num8lora_sender_handle_rx(...)` (ACK/NACK/REQUEST)

Model:
- Oper·cie s˙ log (`op_id` monotÛnne)
- Kaûd˝ receiver m· vlastn˝ progress (`last_acked_op_id`)
- `poll_tx` vyber· receiverov nez·visle (paralelnÈ doruËovanie)

## 2. Receiver strana

Header: `num8lora_receiver.h`

HlavnÈ API:
- `num8lora_receiver_init(...)`
- `num8lora_receiver_handle_beacon(...)` -> vygeneruje `REQUEST`
- `num8lora_receiver_handle_data(...)` -> aplikuje op a vygeneruje `ACK/NACK`
- `num8lora_save_receiver_meta(...)` / `num8lora_load_receiver_meta(...)`

Aplik·cia oper·cie je callback:
- `num8lora_receiver_apply_fn(void* user, uint8_t op_type, uint32_t value)`

Pre NUM8 engine je k dispozÌcii helper callback (pri `NUM8LORA_ENABLE_NUM8`):
- `num8lora_receiver_apply_num8(...)`

## 3. Protokol

Header: `num8lora_op.h`

Spr·vy:
- `BEACON (0x21)`
- `REQUEST (0x22)`
- `DATA (0x23)`
- `ACK (0x24)`
- `NACK (0x25)`

Jedna `DATA` spr·va nesie presne jednu oper·ciu:
- `op_type`: `ADD` alebo `REMOVE`
- `value`: `0..99999999`

## 4. PreËo je to vhodnÈ pre tvoj use-case

Pri 2-3 z·znamoch mesaËne:
- jednoduch˝ tracking kto Ëo uû dostal
- vysok· spoæahlivosù retransmisiÌ
- æahkÈ per-receiver dobiehanie

## 5. Build

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```
