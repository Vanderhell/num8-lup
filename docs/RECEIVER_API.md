# Receiver API Reference

Header: `num8lora_receiver.h`

## Init
- `num8lora_receiver_init(...)`

## Frame processing
- `num8lora_receiver_handle_beacon(...)`
  - ak je update potrebnı, vytvorí REQUEST frame.
- `num8lora_receiver_handle_data(...)`
  - validuje DATA frame, uplatní operáciu cez callback, vytvorí ACK/NACK.

## Apply callback
- typ: `num8lora_receiver_apply_fn(void* user, uint8_t op_type, uint32_t value)`
- musí by deterministickı a rıchly.

## NUM8 helper
- pri `NUM8LORA_ENABLE_NUM8`: `num8lora_receiver_apply_num8(...)`

## Metadata
- `num8lora_save_receiver_meta(...)`
- `num8lora_load_receiver_meta(...)`

## Threading
- Kontext nie je interné thread-safe.
- jeden receiver context obsluhuj jednım worker/event-loopom.
