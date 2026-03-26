# Sender API Reference

Header: `num8lora_sender.h`

## Init
- `num8lora_sender_init(...)`
  - nastaví sender context, timeout/retry politiku a storage buffre.

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
  - non-blocking. keï je pripravený frame, vráti `1` a vyplní `out_target_receiver_id`.
- `num8lora_sender_handle_rx(...)`
  - prijíma REQUEST/ACK/NACK a aktualizuje per-receiver stav.

## Threading
- Kontext nie je interné thread-safe.
- Použi 1 event loop vlákno pre sender context alebo externé locky.
