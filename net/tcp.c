#include <net/tcp.h>
#include <memory.h>
#include <net/skb.h>
#include <assert.h>
#include <serial.h>

int tcp_register_connection(struct sock* sock, uint16_t dst_port, uint16_t src_port)
{
	sock->tcp = kalloc(sizeof(struct tcp_connection));
	memset(sock->tcp, 0, sizeof(struct tcp_connection));
	sock->tcp->dport = dst_port;
	sock->tcp->sport = src_port;
	sock->tcp->state = TCP_CREATED;
	return 0;
}

inline int tcp_is_listening(struct sock* sock)
{
	return sock->tcp->state == TCP_LISTEN;
}

inline int tcp_set_listening(struct sock* sock, int backlog)
{
	
	sock->tcp->backlog = backlog;
	sock->tcp->state = TCP_LISTEN;

	return 1;
}

int tcp_connect(struct sock* sock)
{
	struct sk_buff* skb = skb_new();
	assert(skb != NULL);

	struct tcp_header hdr = {
		.source = sock->bound_port,
		.dest = sock->recv_addr.sin_port,
		.window = 1500,
		.seq = 227728011,
		.ack_seq = 0,
		.doff = 0x05,
		.syn = 1
	};

	if(net_ipv4_add_header(skb, ntohl(sock->recv_addr.sin_addr.s_addr), TCP, sizeof(struct tcp_header)) < 0){
		skb_free(skb);	
		return -1;
	}
	TCP_HTONS(&hdr);

	memcpy(skb->data, &hdr, sizeof(struct tcp_header));

	dbgprintf("Sending TCP %d\n", sizeof(struct tcp_header));

	//((struct udp_header*) skb->data)->checksum = transport_checksum(sip, dip, UDP, (char*) skb->data, htons(length+sizeof(struct udp_header)));
	//((struct udp_header*) skb->data)->checksum = htons(((struct udp_header*) skb->data)->checksum);

	skb->len += sizeof(struct tcp_header);
	skb->data += sizeof(struct tcp_header);

	net_send_skb(skb);
	return 0;
}



int tcp_send_ack(struct sock* sock, uint16_t dst_port, uint16_t src_port)
{
	return 0;
}

int tcp_send_syn(struct sock* sock, uint16_t dst_port, uint16_t src_port)
{	

	return 0;
}

int tcp_recv_ack(struct sock* sock, struct sk_buff* skb)
{
	return -1;
}

int tcp_recv_syn(struct sock* sock, struct sk_buff* skb)
{
	/* Cant really assert this as we dont want to panic if its not true. */
	assert(sock->tcp->state == TCP_LISTEN);
	sock->tcp->state = TCP_SYN_RCVD;

	/* send syn ack & more*/
	return 1;
}


int tcp_parse(struct sk_buff* skb)
{
		/* Look if there is an active TCP connection, if not look for accept. */

		struct tcp_header* hdr = (struct tcp_header* ) skb->data;
		skb->hdr.tcp = hdr;

		struct sock* sk = net_sock_find_tcp(hdr->source, hdr->dest, skb->hdr.ip->saddr);
		if(sk == NULL){
			dbgprintf("[TCP] No socket found for TCP packet while parsing.\n");
			return -1;
		}

		switch (sk->tcp->state)
		{
		case TCP_LISTEN:
			if(hdr->syn == 1 && hdr->ack == 0){
				return tcp_recv_syn(sk, skb);
			}
			break;
		
		case TCP_SYN_RCVD:
			if(hdr->syn == 1 && hdr->ack == 1){
				/* Handle syn / ack */
			}
			break;
		
		case TCP_SYN_SENT:
			if(hdr->syn == 0 && hdr->ack == 1){
				return tcp_recv_ack(sk, skb);
			}
			break;
		
		case TCP_ESTABLISHED:
			/* recv data */
			return tcp_send_ack(0, 0, 0);
			break;
		
		default:
			break;
		}
		
		return -1;
}

void tcp_connection_update()
{
	/* This will be a BIG switch statement, executing functions
	* based on state of a tcp connection
	*/

}

