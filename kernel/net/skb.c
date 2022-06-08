#include <net/skb.h>

struct sk_buff sk_buffers[MAX_SKBUFFERS];

void init_sk_buffers()
{
    for (uint8_t i = 0; i < MAX_SKBUFFERS; i++)
    {
        sk_buffers[i].len = -1;
        sk_buffers[i].data = NULL;
    }   
}