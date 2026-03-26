# NUM8LORA Sender/Receiver Async DLL API

This is the newer split model for one sender and multiple receivers.

## DLL files

- `build/Release/num8lora_sender.dll`
- `build/Release/num8lora_receiver.dll`

## Sender side

Header: `include/num8lora_sender.h`

Main API:

- `num8lora_sender_init(...)`
- `num8lora_sender_register_receiver(...)`
- `num8lora_sender_enqueue_add(...)`
- `num8lora_sender_enqueue_remove(...)`
- `num8lora_sender_enqueue_lists(...)`
- `num8lora_sender_build_beacon(...)`
- `num8lora_sender_poll_tx(...)`
- `num8lora_sender_handle_rx(...)`

Model:

- operations are stored as a monotonic log by `op_id`
- each receiver has independent progress through `last_acked_op_id`
- `poll_tx` schedules delivery separately for each receiver

## Receiver side

Header: `include/num8lora_receiver.h`

Main API:

- `num8lora_receiver_init(...)`
- `num8lora_receiver_handle_beacon(...)`
- `num8lora_receiver_handle_data(...)`
- `num8lora_save_receiver_meta(...)`
- `num8lora_load_receiver_meta(...)`

Operation apply callback:

- `num8lora_receiver_apply_fn(void* user, uint8_t op_type, uint32_t value)`

NUM8 helper:

- `num8lora_receiver_apply_num8(...)` when `NUM8LORA_ENABLE_NUM8` is enabled

## Protocol

Header: `include/num8lora_op.h`

Messages:

- `BEACON (0x21)`
- `REQUEST (0x22)`
- `DATA (0x23)`
- `ACK (0x24)`
- `NACK (0x25)`

Each `DATA` frame carries exactly one operation:

- `op_type`: `ADD` or `REMOVE`
- `value`: `0..99999999`

## Why this model fits small update streams

- simple tracking of who has received which operation
- reliable retransmission behavior
- straightforward per-receiver recovery after downtime
