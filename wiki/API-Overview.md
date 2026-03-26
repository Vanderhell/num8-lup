# API Overview

## Async sender API

Header: `include/num8lora_sender.h`

Key functions:

- `num8lora_sender_init`
- `num8lora_sender_register_receiver`
- `num8lora_sender_set_receiver_progress`
- `num8lora_sender_enqueue_add`
- `num8lora_sender_enqueue_remove`
- `num8lora_sender_enqueue_lists`
- `num8lora_sender_build_beacon`
- `num8lora_sender_poll_tx`
- `num8lora_sender_handle_rx`

## Async receiver API

Header: `include/num8lora_receiver.h`

Key functions:

- `num8lora_receiver_init`
- `num8lora_receiver_handle_beacon`
- `num8lora_receiver_handle_data`
- `num8lora_save_receiver_meta`
- `num8lora_load_receiver_meta`

NUM8 helper:

- `num8lora_receiver_apply_num8` when `NUM8LORA_ENABLE_NUM8` is enabled

## Legacy API

Header: `include/num8lora.h`

The legacy API keeps the original batch-oriented runtime and low-level frame helpers for systems that still depend on the earlier flow.
