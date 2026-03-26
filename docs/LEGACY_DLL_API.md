# NUM8LORA DLL API

This document describes the legacy `num8lora.dll` API and runtime flow that remains compatible with the original batch-oriented update model.

## Binaries

- `build/Release/num8lora.dll`
- `build/Release/num8lora.lib`
- header: `include/num8lora.h`

## Low-level frame API

Use this layer when you provide your own radio or transport:

- CRC: `num8lora_crc16_ccitt_false`
- encode:
  - `num8lora_encode_beacon`
  - `num8lora_encode_update_request`
  - `num8lora_encode_update_data`
  - `num8lora_encode_ack`
  - `num8lora_encode_nack`
- decode:
  - `num8lora_decode_common_header`
  - `num8lora_decode_beacon`
  - `num8lora_decode_update_request`
  - `num8lora_decode_update_data_header`
  - `num8lora_decode_ack`
  - `num8lora_decode_nack`
- validation:
  - `num8lora_validate_frame_crc`
  - `num8lora_validate_update_numbers`
  - `num8lora_decode_update_numbers`
  - `num8lora_compute_update_payload_crc16`

## Sender runtime state machine

Context:

- `num8lora_sender_ctx_t` with `IDLE`, `WAIT_REQUEST`, and `WAIT_ACK`

Main API:

- `num8lora_sender_init`
- `num8lora_sender_set_update`
- `num8lora_sender_build_beacon`
- `num8lora_sender_handle_update_request`
- `num8lora_sender_handle_ack_or_nack`
- `num8lora_sender_on_ack_timeout`

## Receiver runtime state machine

Context:

- `num8lora_receiver_ctx_t` with `SCAN` and `WAIT_UPDATE`

Main API:

- `num8lora_receiver_init`
- `num8lora_receiver_handle_beacon`
- `num8lora_receiver_on_update_timeout`
- `num8lora_receiver_validate_update_data`
- `num8lora_receiver_encode_ack`
- `num8lora_receiver_encode_nack`

## NUM8 compatibility bridge

Requires `NUM8LORA_ENABLE_NUM8` and `num8.h`.

- `num8lora_apply_update_to_num8`
- `num8lora_receiver_apply_update_data_to_num8`

## Receiver metadata persistence

- `num8lora_save_receiver_meta(path, dataset_version, last_applied_update_id)`
- `num8lora_load_receiver_meta(path, &dataset_version, &last_applied_update_id)`
