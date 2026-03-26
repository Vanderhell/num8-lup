#include "num8lora_op.h"

#define HDR 8u
#define CRC 2u

static void st16(uint8_t* p, uint16_t v){ p[0]=(uint8_t)(v&0xFFu); p[1]=(uint8_t)((v>>8)&0xFFu); }
static void st32(uint8_t* p, uint32_t v){ p[0]=(uint8_t)(v&0xFFu); p[1]=(uint8_t)((v>>8)&0xFFu); p[2]=(uint8_t)((v>>16)&0xFFu); p[3]=(uint8_t)((v>>24)&0xFFu); }
static uint16_t ld16(const uint8_t* p){ return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8)); }
static uint32_t ld32(const uint8_t* p){ return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24); }

static void common(uint8_t* out, uint8_t type, uint16_t sid, uint16_t rid, uint16_t seq){ out[0]=NUM8LORA_OP_PROTOCOL_VERSION; out[1]=type; st16(&out[2],sid); st16(&out[4],rid); st16(&out[6],seq); }
static void finish_crc(uint8_t* out, uint32_t len){ st16(&out[len-2], num8lora_op_crc16_ccitt_false(out, len-2)); }

uint16_t num8lora_op_crc16_ccitt_false(const void* data, uint32_t len)
{
    const uint8_t* p=(const uint8_t*)data; uint16_t crc=0xFFFFu; uint32_t i;
    for(i=0;i<len;++i){ int b; crc^=(uint16_t)((uint16_t)p[i]<<8); for(b=0;b<8;++b){ crc=(crc&0x8000u)?(uint16_t)((crc<<1)^0x1021u):(uint16_t)(crc<<1);} }
    return crc;
}

int num8lora_op_encode_beacon(uint8_t* out, uint32_t cap, uint16_t sid, uint16_t seq, const num8lora_op_beacon_payload_t* p, uint32_t* out_len)
{ uint32_t len=HDR+8u+CRC; if(!out||!p||!out_len||cap<len) return 0; common(out,NUM8LORA_OP_MSG_BEACON,sid,0u,seq); st32(&out[8],p->stream_id); st32(&out[12],p->latest_op_id); finish_crc(out,len); *out_len=len; return 1; }
int num8lora_op_encode_request(uint8_t* out, uint32_t cap, uint16_t sid, uint16_t rid, uint16_t seq, const num8lora_op_request_payload_t* p, uint32_t* out_len)
{ uint32_t len=HDR+8u+CRC; if(!out||!p||!out_len||cap<len) return 0; common(out,NUM8LORA_OP_MSG_REQUEST,sid,rid,seq); st32(&out[8],p->stream_id); st32(&out[12],p->next_needed_op_id); finish_crc(out,len); *out_len=len; return 1; }
int num8lora_op_encode_data(uint8_t* out, uint32_t cap, uint16_t sid, uint16_t rid, uint16_t seq, const num8lora_op_data_payload_t* p, uint32_t* out_len)
{ uint32_t len=HDR+16u+CRC; if(!out||!p||!out_len||cap<len||p->reserved0!=0u||p->reserved1!=0u) return 0; common(out,NUM8LORA_OP_MSG_DATA,sid,rid,seq); st32(&out[8],p->stream_id); st32(&out[12],p->op_id); out[16]=p->op_type; out[17]=0u; st16(&out[18],0u); st32(&out[20],p->value); finish_crc(out,len); *out_len=len; return 1; }
int num8lora_op_encode_ack(uint8_t* out, uint32_t cap, uint16_t sid, uint16_t rid, uint16_t seq, const num8lora_op_ack_payload_t* p, uint32_t* out_len)
{ uint32_t len=HDR+8u+CRC; if(!out||!p||!out_len||cap<len) return 0; common(out,NUM8LORA_OP_MSG_ACK,sid,rid,seq); st32(&out[8],p->stream_id); st32(&out[12],p->ack_op_id); finish_crc(out,len); *out_len=len; return 1; }
int num8lora_op_encode_nack(uint8_t* out, uint32_t cap, uint16_t sid, uint16_t rid, uint16_t seq, const num8lora_op_nack_payload_t* p, uint32_t* out_len)
{ uint32_t len=HDR+12u+CRC; if(!out||!p||!out_len||cap<len) return 0; common(out,NUM8LORA_OP_MSG_NACK,sid,rid,seq); st32(&out[8],p->stream_id); st16(&out[12],p->error_code); st16(&out[14],p->detail); st32(&out[16],p->expected_next_op_id); finish_crc(out,len); *out_len=len; return 1; }

int num8lora_op_decode_common_header(const uint8_t* b, uint32_t len, num8lora_op_common_header_t* o)
{ if(!b||!o||len<HDR) return 0; o->protocol_version=b[0]; o->msg_type=b[1]; o->sender_id=ld16(&b[2]); o->receiver_id=ld16(&b[4]); o->seq=ld16(&b[6]); return 1; }
int num8lora_op_validate_frame_crc(const uint8_t* b, uint32_t len)
{ if(!b||len<HDR+CRC) return 0; return ld16(&b[len-2])==num8lora_op_crc16_ccitt_false(b,len-2); }

static int prep(const uint8_t* b, uint32_t len, uint8_t type, num8lora_op_common_header_t* h){ if(!num8lora_op_validate_frame_crc(b,len)||!num8lora_op_decode_common_header(b,len,h)) return 0; return h->protocol_version==NUM8LORA_OP_PROTOCOL_VERSION&&h->msg_type==type; }

int num8lora_op_decode_beacon(const uint8_t* b, uint32_t len, num8lora_op_common_header_t* h, num8lora_op_beacon_payload_t* p)
{ if(!b||!h||!p||len!=18u||!prep(b,len,NUM8LORA_OP_MSG_BEACON,h)||h->receiver_id!=0u) return 0; p->stream_id=ld32(&b[8]); p->latest_op_id=ld32(&b[12]); return 1; }
int num8lora_op_decode_request(const uint8_t* b, uint32_t len, num8lora_op_common_header_t* h, num8lora_op_request_payload_t* p)
{ if(!b||!h||!p||len!=18u||!prep(b,len,NUM8LORA_OP_MSG_REQUEST,h)) return 0; p->stream_id=ld32(&b[8]); p->next_needed_op_id=ld32(&b[12]); return 1; }
int num8lora_op_decode_data(const uint8_t* b, uint32_t len, num8lora_op_common_header_t* h, num8lora_op_data_payload_t* p)
{ if(!b||!h||!p||len!=26u||!prep(b,len,NUM8LORA_OP_MSG_DATA,h)) return 0; p->stream_id=ld32(&b[8]); p->op_id=ld32(&b[12]); p->op_type=b[16]; p->reserved0=b[17]; p->reserved1=ld16(&b[18]); p->value=ld32(&b[20]); return p->reserved0==0u&&p->reserved1==0u; }
int num8lora_op_decode_ack(const uint8_t* b, uint32_t len, num8lora_op_common_header_t* h, num8lora_op_ack_payload_t* p)
{ if(!b||!h||!p||len!=18u||!prep(b,len,NUM8LORA_OP_MSG_ACK,h)) return 0; p->stream_id=ld32(&b[8]); p->ack_op_id=ld32(&b[12]); return 1; }
int num8lora_op_decode_nack(const uint8_t* b, uint32_t len, num8lora_op_common_header_t* h, num8lora_op_nack_payload_t* p)
{ if(!b||!h||!p||len!=22u||!prep(b,len,NUM8LORA_OP_MSG_NACK,h)) return 0; p->stream_id=ld32(&b[8]); p->error_code=ld16(&b[12]); p->detail=ld16(&b[14]); p->expected_next_op_id=ld32(&b[16]); return 1; }
