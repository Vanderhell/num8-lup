# Deployment Checklist

## Build

- [ ] `cmake -S . -B build`
- [ ] `cmake --build build --config Release`
- [ ] `ctest --test-dir build -C Release --output-on-failure`

## Artifacts

- [ ] `num8lora_sender.dll`
- [ ] `num8lora_receiver.dll`
- [ ] `num8lora.dll` if the legacy compatibility library is part of the release
- [ ] matching `.lib` files
- [ ] public headers from `include/`

## Sender runtime

- [ ] receiver registry initialized
- [ ] `ack_timeout_ms` and `max_retries` configured
- [ ] `last_acked_op_id` monitored for each receiver

## Receiver runtime

- [ ] `last_applied_op_id` persisted through `num8lora_save_receiver_meta`
- [ ] state restored on startup through `num8lora_load_receiver_meta`
- [ ] `stream_id` and `sender_id` validated

## NUM8 integration

- [ ] apply callback is idempotent
- [ ] NUM8 flush policy is defined
- [ ] `NACK` error telemetry is captured
