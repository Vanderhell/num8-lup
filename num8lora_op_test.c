#include "num8lora_op.h"
#include <stdio.h>

#define C(x) do { if(!(x)) { fprintf(stderr,"FAIL %d\n",__LINE__); return 1; } } while(0)

int main(void)
{
    uint8_t b[64];
    uint32_t len=0;
    num8lora_op_common_header_t h;
    num8lora_op_data_payload_t in_d, out_d;

    in_d.stream_id=7u; in_d.op_id=11u; in_d.op_type=NUM8LORA_OP_ADD; in_d.reserved0=0u; in_d.reserved1=0u; in_d.value=12345678u;
    C(num8lora_op_encode_data(b,sizeof(b),10u,22u,1u,&in_d,&len));
    C(len==26u);
    C(num8lora_op_decode_data(b,len,&h,&out_d));
    C(h.sender_id==10u && h.receiver_id==22u);
    C(out_d.stream_id==in_d.stream_id && out_d.op_id==in_d.op_id && out_d.value==in_d.value);

    b[5]^=0x10u;
    C(!num8lora_op_validate_frame_crc(b,len));

    printf("num8lora_op_test OK\n");
    return 0;
}
