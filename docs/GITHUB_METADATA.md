# GitHub Metadata

Use this file as the source of truth when creating or updating the GitHub repository page.

## Repository name

`num8-lup`

## Short description

Reliable C library for NUM8 update propagation over low-bandwidth LoRa-style links, with both legacy batch and modern async sender/receiver flows.

## Topics

- `c`
- `cmake`
- `dll`
- `lora`
- `low-bandwidth`
- `embedded`
- `protocol`
- `replication`
- `data-sync`
- `async`
- `windows`
- `num8`

## Suggested About blurb

NUM8-LUP is a compact C implementation of a low-bandwidth update protocol designed for small, infrequent dataset changes. It ships a legacy batch-compatible API and a newer async sender/receiver split model with per-receiver progress tracking, retransmissions, and NUM8 integration hooks.

## Suggested README tagline

Low-bandwidth NUM8 update propagation in C, built for small delta streams and unreliable links.

## Release artifacts to attach

- `num8lora_sender.dll`
- `num8lora_sender.lib`
- `num8lora_receiver.dll`
- `num8lora_receiver.lib`
- `num8lora.dll`
- `num8lora.lib`
- public headers:
  - `num8lora.h`
  - `num8lora_op.h`
  - `num8lora_sender.h`
  - `num8lora_receiver.h`

## Repository settings to review before publishing

- Set repository description and topics from this file.
- Enable GitHub Wiki if you want the prepared wiki pages published.
- Add a license file before making the repository public.
- Decide whether legacy API should be marked as stable, deprecated, or compatibility-only in the first public release.
- Create the first tagged release only after `ctest` passes from a clean build directory.
