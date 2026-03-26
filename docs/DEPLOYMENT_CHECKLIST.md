# Deployment Checklist

## Build
- [ ] `cmake -S . -B build`
- [ ] `cmake --build build --config Release`
- [ ] `ctest --test-dir build -C Release --output-on-failure`

## Artifacts
- [ ] `num8lora_sender.dll`
- [ ] `num8lora_receiver.dll`
- [ ] príslušné `.lib` a header súbory (`num8lora_op.h`, `num8lora_sender.h`, `num8lora_receiver.h`)

## Sender runtime
- [ ] inicializované registry receiverov
- [ ] nastavený `ack_timeout_ms` a `max_retries`
- [ ] monitoring `last_acked_op_id` pre každý receiver

## Receiver runtime
- [ ] persist `last_applied_op_id` cez `num8lora_save_receiver_meta`
- [ ] load stavu pri štarte cez `num8lora_load_receiver_meta`
- [ ] kontrola stream_id/sender_id

## NUM8 integration
- [ ] apply callback je idempotentný
- [ ] definovaná flush politika NUM8 engine
- [ ] error telemetry pre NACK kódy
