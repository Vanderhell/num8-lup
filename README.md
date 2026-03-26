# NUM8-LUP

Low-bandwidth NUM8 update propagation in C, built for small delta streams and unreliable links.

NUM8-LUP is a compact C project for propagating small NUM8 dataset changes over LoRa-style or similarly constrained transports. The repository exposes both a legacy batch-compatible library and a newer async sender/receiver split protocol for deployments that need cleaner per-receiver progress tracking.

## Project Scope

The project currently ships two protocol lines:

1. Legacy batch API for compatibility with the original update flow.
2. Async split API for single-operation delivery, retransmissions, and receiver catch-up.

For new deployments, the async split model is the recommended default.

## Repository Layout

```text
.
|-- include/   public headers
|-- src/       library implementations
|-- tests/     verification targets
|-- examples/  sample usage
|-- docs/      protocol and publishing references
|-- wiki/      prepared GitHub wiki pages
|-- CMakeLists.txt
`-- README.md
```

This layout keeps the public API, implementation, tests, and documentation clearly separated.

## Public API Surface

Main headers:

- `include/num8lora.h`
- `include/num8lora_op.h`
- `include/num8lora_sender.h`
- `include/num8lora_receiver.h`

Release outputs:

- `num8lora.dll`
- `num8lora_sender.dll`
- `num8lora_receiver.dll`

### Legacy Batch API

The legacy library is declared in `include/num8lora.h` and implemented in:

- `src/num8lora.c`
- `src/num8lora_runtime.c`
- `src/num8lora_meta.c`
- `src/num8lora_num8_bridge.c`

It provides:

- low-level frame encoding and decoding helpers
- sender and receiver runtime state machines
- receiver metadata persistence
- optional NUM8 bridge integration

### Async Split API

The async split model is declared in:

- `include/num8lora_op.h`
- `include/num8lora_sender.h`
- `include/num8lora_receiver.h`

Implementation mapping:

- `src/num8lora_op.c` handles frame encoding, decoding, and CRC validation
- `src/num8lora_sender.c` manages the sender queue, per-receiver state, and retransmissions
- `src/num8lora_receiver.c` handles beacon processing, operation apply flow, and ACK/NACK generation

## Async Split Model

The async sender/receiver flow is designed for small, infrequent changes where delivery reliability matters more than throughput.

Sender side:

- tracks each receiver independently
- stores `ADD` and `REMOVE` operations in a monotonic log
- emits `BEACON` frames and schedules `DATA` retransmissions
- consumes inbound `REQUEST`, `ACK`, and `NACK` frames

Receiver side:

- requests only the next missing operation
- applies one operation per `DATA` frame
- persists metadata across restarts
- can apply changes into a NUM8 engine through a callback helper

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

## Tests

Core test targets:

- `num8lora_test`
- `num8lora_runtime_test`
- `num8lora_op_test`
- `num8lora_async_test`

Additional NUM8-dependent tests:

- `tests/num8lora_num8_test.c`
- `tests/num8lora_flow_num8_test.c`

Optional NUM8 targets are enabled only when `../NUM8-DENSE` is available during CMake configure.

## Documentation Map

Primary docs:

- `docs/QUICKSTART.md`
- `docs/SENDER_API.md`
- `docs/RECEIVER_API.md`
- `docs/WIRE_FORMAT.md`
- `docs/DEPLOYMENT_CHECKLIST.md`

Long-form technical reference:

- `docs/ASYNC_SPLIT_API.md`
- `docs/LEGACY_DLL_API.md`
- `docs/SPECIFICATION.md`

GitHub publishing assets:

- `docs/GITHUB_METADATA.md`
- `docs/PUBLISHING_CHECKLIST.md`
- `wiki/`

## Repository Notes

- The project uses C99 and CMake.
- Shared libraries are configured with `WINDOWS_EXPORT_ALL_SYMBOLS ON`.
- Add a `LICENSE` file before making the repository public.
