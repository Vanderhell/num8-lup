# Protocol

The async split protocol uses a compact frame format with CRC-16/CCITT-FALSE protection.

## Message types

- `BEACON (0x21)`
- `REQUEST (0x22)`
- `DATA (0x23)`
- `ACK (0x24)`
- `NACK (0x25)`

## DATA payload model

Each `DATA` frame carries exactly one operation:

- `op_id`
- `op_type`
- `value`

`op_type` values:

- `1 = ADD`
- `2 = REMOVE`

`value` range:

- `0..99999999`

## Delivery model

- sender advertises progress through `BEACON`
- receiver requests the next missing operation
- sender transmits single-operation `DATA`
- receiver responds with `ACK` or `NACK`
- progress is tracked independently for each receiver
