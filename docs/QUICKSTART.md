# Quickstart

## Build

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Linux:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
cmake --install build --prefix /usr/local
```

## DLL outputs

- `build/Release/num8lora_sender.dll`
- `build/Release/num8lora_receiver.dll`
- `build/Release/num8lora.dll`

## Sender bootstrap

1. Call `num8lora_sender_init`.
2. Register each receiver with `num8lora_sender_register_receiver`.
3. Enqueue operations with `num8lora_sender_enqueue_add`, `num8lora_sender_enqueue_remove`, or `num8lora_sender_enqueue_lists`.
4. Emit periodic `num8lora_sender_build_beacon` frames.
5. In the event loop, feed inbound frames to `num8lora_sender_handle_rx`.
6. Poll outbound `DATA` frames through `num8lora_sender_poll_tx`.

## Receiver bootstrap

1. Call `num8lora_receiver_init`.
2. After a beacon, call `num8lora_receiver_handle_beacon` to generate `REQUEST`.
3. After a `DATA` frame, call `num8lora_receiver_handle_data` to apply and emit `ACK` or `NACK`.
4. Persist state with `num8lora_save_receiver_meta` and restore it with `num8lora_load_receiver_meta`.

## NUM8 engine apply

- Pass `num8lora_receiver_apply_num8` and a `num8_engine_t*` user pointer into `num8lora_receiver_handle_data`.
- Choose a commit policy that matches your deployment, for example flush on every operation or periodic flush.

## Demo

- source: `examples/async_demo.c`
- target: `num8lora_async_demo`
