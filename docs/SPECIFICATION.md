# NUM8 LoRa Update Protocol Specification

This document summarizes the original batch-oriented NUM8 update protocol that the legacy `num8lora` API still supports.

## Purpose

The protocol transfers incremental updates of a set of unique 8-digit numbers between:

- a sender
- a receiver

Each update contains only:

- numbers to remove
- numbers to add

The design goals are:

- simple binary framing
- C-friendly implementation
- support for small batched updates
- duplicate frame tolerance
- corrupted frame detection
- dataset version validation before apply

## Message types

- `BEACON`
- `UPDATE_REQUEST`
- `UPDATE_DATA`
- `ACK`
- `NACK`

## Frame model

Each frame contains:

- common header
- message-specific payload
- CRC16 footer

The receiver validates:

- protocol version
- expected message type
- receiver id
- payload consistency
- CRC integrity

## Batch update flow

1. Sender publishes a `BEACON` describing the currently available update.
2. Receiver checks local dataset version and decides whether it needs the update.
3. Receiver sends `UPDATE_REQUEST`.
4. Sender responds with `UPDATE_DATA`.
5. Receiver validates and applies the batch.
6. Receiver responds with `ACK` or `NACK`.

## Batch payload rules

- values must stay inside the valid NUM8 domain
- duplicate values inside one remove list are invalid
- duplicate values inside one add list are invalid
- the same value cannot appear in both remove and add sections of one batch

## Error handling

The legacy API maps validation failures to explicit `NUM8LORA_ERR_*` error codes so the caller can emit `NACK` frames consistently.

## Implementation mapping

- header: `include/num8lora.h`
- codec: `src/num8lora.c`
- sender and receiver runtime: `src/num8lora_runtime.c`
- metadata persistence: `src/num8lora_meta.c`
- optional NUM8 bridge: `src/num8lora_num8_bridge.c`
