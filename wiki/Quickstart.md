# Quickstart

## Build

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Main outputs

- `build/Release/num8lora_sender.dll`
- `build/Release/num8lora_receiver.dll`
- `build/Release/num8lora.dll`

## Async sender flow

1. Initialize a sender context.
2. Register each receiver.
3. Enqueue add/remove operations.
4. Emit periodic `BEACON` frames.
5. Feed inbound `REQUEST` and `ACK/NACK` frames back into the sender.
6. Poll outbound `DATA` frames from the sender scheduler.

## Async receiver flow

1. Initialize a receiver context.
2. Consume `BEACON` frames and emit `REQUEST` when behind.
3. Consume `DATA` frames and apply each operation through a callback.
4. Persist metadata with `num8lora_save_receiver_meta` and restore it on startup.

## Demo

- source: `examples/async_demo.c`
- target: `num8lora_async_demo`
