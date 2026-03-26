#include "num8lora_sender.h"
#include "num8lora_receiver.h"

#include <stdio.h>

#define C(x) do { if(!(x)) { fprintf(stderr,"FAIL %d\n",__LINE__); return 1; } } while(0)

typedef struct state_s { int present[1000]; } state_t;

static int apply_fn(void* u, uint8_t op_type, uint32_t value)
{
    state_t* s=(state_t*)u;
    if(value>=1000u) return 0;
    if(op_type==NUM8LORA_OP_ADD) s->present[value]=1;
    else if(op_type==NUM8LORA_OP_REMOVE) s->present[value]=0;
    else return 0;
    return 1;
}

int main(void)
{
    num8lora_sender_t snd;
    num8lora_sender_op_t ops[64];
    num8lora_sender_receiver_slot_t slots[8];

    num8lora_receiver_t r1, r2;
    state_t s1={0}, s2={0};

    uint8_t beacon[64], req[64], tx[128], resp[128];
    uint32_t bl=0, rl=0, tl=0, rpl=0;
    uint16_t target=0;
    uint32_t first=0,last=0;
    uint64_t now=0;
    int guard=0;

    num8lora_sender_init(&snd,10u,1001u,50u,3u,ops,64u,slots,8u);
    C(num8lora_sender_register_receiver(&snd,21u,0u));
    C(num8lora_sender_register_receiver(&snd,22u,0u));

    num8lora_receiver_init(&r1,21u,10u,1001u,0u);
    num8lora_receiver_init(&r2,22u,10u,1001u,0u);

    C(num8lora_sender_enqueue_lists(&snd,(uint32_t[]){5u},1u,(uint32_t[]){7u,9u},2u,&first,&last));
    C(first==1u && last==3u);

    C(num8lora_sender_build_beacon(&snd,beacon,sizeof(beacon),&bl));

    C(num8lora_receiver_handle_beacon(&r1,beacon,bl,req,sizeof(req),&rl));
    C(num8lora_sender_handle_rx(&snd,now,req,rl));
    C(num8lora_receiver_handle_beacon(&r2,beacon,bl,req,sizeof(req),&rl));
    C(num8lora_sender_handle_rx(&snd,now,req,rl));

    while(guard++<200)
    {
        if(!num8lora_sender_poll_tx(&snd,now,tx,sizeof(tx),&tl,&target))
        {
            now+=10;
            continue;
        }

        if(target==21u)
        {
            C(num8lora_receiver_handle_data(&r1,tx,tl,apply_fn,&s1,resp,sizeof(resp),&rpl));
            C(num8lora_sender_handle_rx(&snd,now,resp,rpl));
        }
        else if(target==22u)
        {
            if(now<30u)
            {
                now+=60;
                continue;
            }
            C(num8lora_receiver_handle_data(&r2,tx,tl,apply_fn,&s2,resp,sizeof(resp),&rpl));
            C(num8lora_sender_handle_rx(&snd,now,resp,rpl));
        }

        now+=5;

        if(r1.last_applied_op_id==3u && r2.last_applied_op_id==3u)
        {
            break;
        }
    }

    C(r1.last_applied_op_id==3u);
    C(r2.last_applied_op_id==3u);
    C(s1.present[5]==0 && s1.present[7]==1 && s1.present[9]==1);
    C(s2.present[5]==0 && s2.present[7]==1 && s2.present[9]==1);

    printf("num8lora_async_test OK\n");
    return 0;
}
