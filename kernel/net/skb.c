#include <net/skb.h>

struct sk_buff sk_buffers[MAX_SKBUFFERS];

void init_sk_buffers()
{
	for (uint8_t i = 0; i < MAX_SKBUFFERS; i++)
	{
		FREE_SKB(&sk_buffers[i]);
	}
}

struct sk_buff* get_skb()
{
	int16_t i;
	for (i = 0; i < MAX_SKBUFFERS; i++)
		if(sk_buffers[i].len == -1)
			return &sk_buffers[i];

	return NULL;
}

