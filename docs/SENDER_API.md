# Sender API Reference

Header: `include/num8lora_sender.h`

## Init

- `num8lora_sender_init(...)`
  - sets sender context, timeout and retry policy, and caller-owned buffers

## Receiver registry

- `num8lora_sender_register_receiver(...)`
- `num8lora_sender_set_receiver_progress(...)`

## Enqueue operations

- `num8lora_sender_enqueue_add(...)`
- `num8lora_sender_enqueue_remove(...)`
- `num8lora_sender_enqueue_lists(...)`

## Transport hooks

- `num8lora_sender_build_beacon(...)`
- `num8lora_sender_poll_tx(...)`
  - non-blocking; when a frame is ready it returns `1` and fills `out_target_receiver_id`
- `num8lora_sender_handle_rx(...)`
  - consumes `REQUEST`, `ACK`, and `NACK` frames and updates per-receiver state

## Threading

- context is not internally thread-safe
- drive a sender context from one event loop thread or protect it with external locking
