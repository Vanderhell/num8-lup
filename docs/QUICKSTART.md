# Quickstart (Async Split)

## 1) Build

```powershell
cmake -S . -B build
cmake --build build --config Release
```

## 2) DLL outputs

- `build/Release/num8lora_sender.dll`
- `build/Release/num8lora_receiver.dll`

## 3) Sender bootstrap

1. `num8lora_sender_init`
2. `num8lora_sender_register_receiver` pre každý receiver
3. `num8lora_sender_enqueue_add/remove` alebo `num8lora_sender_enqueue_lists`
4. periodicky `num8lora_sender_build_beacon`
5. v event loope:
   - `num8lora_sender_handle_rx` (REQUEST/ACK/NACK)
   - `num8lora_sender_poll_tx` (DATA/resend)

## 4) Receiver bootstrap

1. `num8lora_receiver_init`
2. po beacone vola `num8lora_receiver_handle_beacon` -> REQUEST
3. po DATA vola `num8lora_receiver_handle_data` -> ACK/NACK
4. metadata persist cez `num8lora_save_receiver_meta`/`num8lora_load_receiver_meta`

## 5) NUM8 engine apply

- do `num8lora_receiver_handle_data` pošli callback `num8lora_receiver_apply_num8` a `num8_engine_t*`.
- commit pod¾a tvojej politiky (napr. flush po každej operácii, alebo periodicky).

## 6) Demo

- source: `examples/async_demo.c`
- cie¾: `num8lora_async_demo`
