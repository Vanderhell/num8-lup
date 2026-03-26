# Wire Format (Single-Op)

Header file: `include/num8lora_op.h`

Header (8 bytes):

- `protocol_version` (1)
- `msg_type` (1)
- `sender_id` (2, LE)
- `receiver_id` (2, LE)
- `seq` (2, LE)

Footer:

- `crc16` (2, CRC-16/CCITT-FALSE over the whole frame without the CRC field)

Message payloads:

- `BEACON(0x21)`: `stream_id:u32`, `latest_op_id:u32`
- `REQUEST(0x22)`: `stream_id:u32`, `next_needed_op_id:u32`
- `DATA(0x23)`: `stream_id:u32`, `op_id:u32`, `op_type:u8`, `reserved0:u8`, `reserved1:u16`, `value:u32`
- `ACK(0x24)`: `stream_id:u32`, `ack_op_id:u32`
- `NACK(0x25)`: `stream_id:u32`, `error_code:u16`, `detail:u16`, `expected_next_op_id:u32`

`op_type`:

- `1 = ADD`
- `2 = REMOVE`

`value` range:

- `0..99999999`
