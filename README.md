# NUM8-LUP

Low-bandwidth NUM8 update propagation in C, built for small delta streams and unreliable links.

NUM8-LUP provides a compact transport-friendly update layer for propagating small dataset changes over LoRa-style or similarly constrained links. The repository includes both a legacy batch-compatible library and a newer async sender/receiver split model optimized for infrequent updates and per-receiver catch-up.

## Project Scope

The project currently ships two protocol lines:

1. Legacy batch API in `num8lora.*`, kept for compatibility with the original specification and existing integrations.
2. Async split API built around single-operation delivery, explicit sender/receiver roles, and independent receiver progress tracking.

For new deployments, the async split model is the recommended default.

## Main Outputs

Shared libraries built in Release mode:

- `num8lora.dll`
- `num8lora_sender.dll`
- `num8lora_receiver.dll`

Main public headers:

- `num8lora.h`
- `num8lora_op.h`
- `num8lora_sender.h`
- `num8lora_receiver.h`

## Async Split Model

The modern sender/receiver flow is designed for small, infrequent changes where reliable delivery matters more than throughput.

Sender side:

- tracks each receiver independently
- enqueues `ADD` and `REMOVE` operations as a monotonic log
- emits `BEACON` frames and schedules `DATA` retransmissions
- consumes inbound `REQUEST`, `ACK`, and `NACK` frames

Receiver side:

- requests only the next missing operation
- applies one operation per `DATA` frame
- persists receiver metadata across restarts
- can apply updates into a NUM8 engine through a callback helper

Protocol messages:

- `BEACON`
- `REQUEST`
- `DATA`
- `ACK`
- `NACK`

Each `DATA` frame carries exactly one operation with a value in the range `0..99999999`.

## Build

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Example

The repository includes an end-to-end async example:

- source: `examples/async_demo.c`
- target: `num8lora_async_demo`

## Documentation Map

Public docs:

- `docs/QUICKSTART.md`
- `docs/SENDER_API.md`
- `docs/RECEIVER_API.md`
- `docs/WIRE_FORMAT.md`
- `docs/DEPLOYMENT_CHECKLIST.md`

Protocol/API reference:

- `NUM8LORA_ASYNC_SPLIT_API.md`
- `NUM8LORA_DLL_API.md`
- `Specification..md`

GitHub publishing assets:

- `docs/GITHUB_METADATA.md`
- `docs/PUBLISHING_CHECKLIST.md`
- `wiki/`

## Tests

Core test targets:

- `num8lora_test`
- `num8lora_runtime_test`
- `num8lora_op_test`
- `num8lora_async_test`

Optional NUM8-dependent targets are enabled only when `../NUM8-DENSE` is available during CMake configure.

## Repository Notes

- The project uses C99 and CMake.
- Shared libraries are configured with `WINDOWS_EXPORT_ALL_SYMBOLS ON`.
- The repository should get a `LICENSE` file before being made public.
