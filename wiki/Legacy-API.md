# Legacy API

The legacy `num8lora` library remains useful when you need compatibility with the earlier batch-based protocol and runtime state machines.

Header:

- `include/num8lora.h`

Implementation files:

- `src/num8lora.c`
- `src/num8lora_runtime.c`
- `src/num8lora_meta.c`
- `src/num8lora_num8_bridge.c`

It provides:

- low-level frame encoding and decoding helpers
- sender and receiver runtime contexts for the batch flow
- NUM8 bridge helpers for legacy integration
- metadata persistence helpers for receiver state

For modern greenfield deployments, the async split sender/receiver model is the cleaner default because it exposes explicit per-receiver progress and single-operation retransmission.
