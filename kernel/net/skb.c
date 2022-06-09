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

int get_skb(char* buffer, uint16_t size, struct sk_buff* skb)
{
    skb = NULL;

    for (uint8_t i = 0; i < MAX_SKBUFFERS; i++)
    {
        if(sk_buffers[i].len == -1)
        {
            skb = &sk_buffers[i];
            break;
        }
    }

    if(skb == NULL)
        return -1;

    ALLOCATE_SKB(buffer, size, skb);
    
}

