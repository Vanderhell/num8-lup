# Receiver API Reference

Header: `include/num8lora_receiver.h`

## Init

- `num8lora_receiver_init(...)`

## Frame processing

- `num8lora_receiver_handle_beacon(...)`
  - generates a `REQUEST` frame when the receiver is behind
- `num8lora_receiver_handle_data(...)`
  - validates a `DATA` frame, applies the operation through a callback, and generates `ACK` or `NACK`

## Apply callback

- type: `num8lora_receiver_apply_fn(void* user, uint8_t op_type, uint32_t value)`
- should be deterministic and fast

## NUM8 helper

- when `NUM8LORA_ENABLE_NUM8` is enabled: `num8lora_receiver_apply_num8(...)`

## Metadata

- `num8lora_save_receiver_meta(...)`
- `num8lora_load_receiver_meta(...)`

## Threading

- context is not internally thread-safe
- handle one receiver context from one worker or event loop
