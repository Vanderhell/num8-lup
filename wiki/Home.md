# NUM8-LUP

NUM8-LUP is a compact C project for propagating small NUM8 dataset updates over low-bandwidth and unreliable links such as LoRa-style transports.

The project currently exposes two protocol models:

- a legacy batch-compatible API
- a newer async split model with dedicated sender and receiver libraries

## Repository layout

- `include/` public headers
- `src/` implementations
- `tests/` verification targets
- `examples/` sample integration
- `docs/` protocol and publishing references

## What the async split model gives you

- per-receiver progress tracking
- single-operation `DATA` messages
- retransmission-friendly flow
- straightforward recovery after receiver restarts
- NUM8 integration through an apply callback

## Entry points

- Build: see [[Quickstart]]
- Protocol summary: see [[Protocol]]
- Public API overview: see [[API Overview]]
- Legacy compatibility notes: see [[Legacy API]]
- Release preparation: see [[Publishing]]
