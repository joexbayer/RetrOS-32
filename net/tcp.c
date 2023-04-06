#include <net/tcp.h>
#include <memory.h>
#include <net/skb.h>
#include <net/dhcp.h>
#include <net/net.h>
#include <assert.h>
#include <serial.h>
#include <rbuffer.h>
#include <scheduler.h>

int tcp_new_connection(struct sock* sock, uint16_t dst_port, uint16_t src_port)
{
	sock->tcp = kalloc(sizeof(struct tcp_connection));
	memset(sock->tcp, 0, sizeof(struct tcp_connection));
	sock->tcp->dport = dst_port;
	sock->tcp->sport = src_port;
	sock->tcp->state = TCP_CREATED;
	sock->tcp->sequence = 227728011;

	return 0;
}

int tcp_free_connection(struct sock* sock)
{
	/* TODO: check for active connections */
	kfree(sock->tcp);
	sock->tcp = NULL;

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

uint16_t tcp_calculate_checksum(uint32_t src_ip, uint32_t dest_ip, unsigned short *data, int size)
{
    register unsigned long sum = 0;
    unsigned short tcpLen = size;
    struct tcp_header *tcphdrp = (struct tcp_header*)(data);

    /* the source ip */
    sum += (src_ip>>16)&0xFFFF;
    sum += (src_ip)&0xFFFF;
    /* the dest ip */
    sum += (dest_ip>>16)&0xFFFF;
    sum += (dest_ip)&0xFFFF;
    /* protocol and reserved: 6 */
    sum += htons(TCP);
    /* the length  */
    sum += htons(tcpLen);
 
    /* add the IP payload */
    /* initialize checksum to 0 */
    tcphdrp->check = 0;
    while (tcpLen > 1) {
        sum += * data++;
        tcpLen -= 2;
    }
    /* if any bytes left, pad the bytes and add */
    if(tcpLen > 0) {
        sum += ((*data)&htons(0xFF00));
    }
      /* Fold 32-bit sum to 16 bits: add carrier to result */
      while (sum>>16) {
          sum = (sum & 0xffff) + (sum >> 16);
      }
      sum = ~sum;

	return sum;
}

static int __tcp_send(struct sock* sock, struct tcp_header* hdr, struct sk_buff* skb, uint8_t* data, uint32_t len)
{
	if(net_ipv4_add_header(skb, ntohl(sock->recv_addr.sin_addr.s_addr), TCP, sizeof(struct tcp_header)+len) < 0){
		skb_free(skb);	
		return -1;
	}
	TCP_HTONS(hdr);

	memcpy(skb->data, hdr, sizeof(struct tcp_header));
	hdr = (struct tcp_header*) skb->data;

	dbgprintf("Sending TCP %d\n", sizeof(struct tcp_header));

	skb->len += sizeof(struct tcp_header);
	skb->data += sizeof(struct tcp_header);

	if(len > 0){
		memcpy(skb->data, data, len);
		skb->len += len;
		skb->data += len;
	}

	hdr->check = tcp_calculate_checksum(dhcp_get_ip(), sock->recv_addr.sin_addr.s_addr, (unsigned short*)hdr, sizeof(struct tcp_header)+len);

	net_send_skb(skb);

	return 0;
}

int tcp_send_segment(struct sock* sock, uint8_t* data, uint32_t len, uint8_t push)
{
	struct sk_buff* skb = skb_new();
	assert(skb != NULL);

	dbgprintf("[TCP] Sending segment with size %d, seq: %d (%d after)\n", len, sock->tcp->sequence, sock->tcp->sequence+len);
	struct tcp_header hdr = {
		.source = sock->bound_port,
		.dest = sock->recv_addr.sin_port,
		.window = 1500,
		.seq = sock->tcp->sequence,
		.ack_seq = sock->tcp->acknowledgement,
		.doff = 0x05,
		.ack = 1,
		.psh = push
	};

	sock->tcp->sequence += len;

	__tcp_send(sock, &hdr, skb, data, len);
	return 0;
}

int tcp_send_ack(struct sock* sock, struct tcp_header* tcp, int len)
{
	struct sk_buff* skb = skb_new();
	assert(skb != NULL);

	struct tcp_header hdr = {
		.source = tcp->dest,
		.dest = tcp->source,
		.window = 1500,
		.seq = htonl(tcp->ack_seq),
		.ack_seq = htonl(tcp->seq)+len,
		.doff = 0x05,
		.ack = 1
	};

	sock->tcp->sequence = htonl(tcp->ack_seq);
	sock->tcp->acknowledgement = htonl(tcp->seq)+1;

	__tcp_send(sock, &hdr, skb, NULL, 0);
	return 0;
}

/* Currently deprecated */
int tcp_read(struct sock* sock, uint8_t* buffer, unsigned int length)
{
	/*
	dbgprintf(" [TCP] Waiting for data... %d\n", sock);
	// Should be blocking 
	WAIT(!net_sock_data_ready(sock, length));

	int to_read = length > sock->tcp->recvd ? sock->tcp->recvd : length;
	int ret = sock->tcp->recv_buffer->ops->read(sock->tcp->recv_buffer, buffer, to_read);
	if(ret != to_read){
		dbgprintf("[TCP] Read from recv buffer not equal to return value!\n");
	}
	sock->tcp->recvd -= to_read;

	dbgprintf("[TCP] Received %d from socket %d\n", to_read, sock);

	return to_read;
	*/

	return -1;
}

/* Currently deprecated */
int tcp_recv_segment(struct sock* sock, struct tcp_header* tcp, struct sk_buff* skb)
{
	/* TODO: if data_ready == 1 wait for it to be cleared. 

	int ret = sock->tcp->recv_buffer->ops->add(sock->tcp->recv_buffer, skb->data, skb->data_len);
	if(ret < 0){
		dbgprintf("[TCP] recv ring buffer is full!\n");
		return ret;
	}
	sock->tcp->recvd += skb->data_len;
	sock->tcp->data_ready = tcp->psh;

	dbgprintf("[TCP] Added segment to socket %d (ready: %d)\n", sock, sock->tcp->data_ready);

	tcp_send_ack(sock, tcp, skb->data_len);
	return 0;
	*/

	return -1;
}


int tcp_connect(struct sock* sock)
{
	struct sk_buff* skb = skb_new();
	assert(skb != NULL);

	struct tcp_header hdr = {
		.source = sock->bound_port,
		.dest = sock->recv_addr.sin_port,
		.window = 1500,
		.seq = sock->tcp->sequence,
		.ack_seq = 0,
		.doff = 0x05,
		.syn = 1
	};

	__tcp_send(sock, &hdr, skb, NULL, 0);
	return 0;
}

int tcp_send_syn(struct sock* sock, uint16_t dst_port, uint16_t src_port)
{

	return 0;
}

int tcp_recv_ack(struct sock* sock, struct tcp_header* tcp)
{
	dbgprintf("[TCP] Incoming TCP ack expected %d got %d\n", sock->tcp->sequence, htonl(tcp->ack_seq));
	if(sock->tcp->sequence == htonl(tcp->ack_seq)){
		dbgprintf("[TCP] Correct sequence acked\n");
		sock->tcp->acknowledgement = htonl(tcp->seq)+1;
		sock->tcp->state = TCP_ESTABLISHED;
	}
	
	return 0;
}

int tcp_recv_fin(struct sock* sock, struct tcp_header* tcp)
{
	/* send fin/ack -> close socket*/
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

int tcp_send_fin(struct sock* sock, struct tcp_header* tcp)
{
	/* send fin (ack?) */
	return 0;
}


int tcp_parse(struct sk_buff* skb)
{
		/* Look if there is an active TCP connection, if not look for accept. */

	struct tcp_header* hdr = (struct tcp_header* ) skb->data;
	skb->hdr.tcp = hdr;
	skb->data += hdr->doff*4;
	skb->data_len = skb->hdr.ip->len - skb->hdr.ip->ihl*4 - hdr->doff*4;

	struct sock* sk = net_sock_find_tcp(hdr->source, hdr->dest, htonl(skb->hdr.ip->saddr));
	if(sk == NULL){
		dbgprintf("[TCP] No socket found for TCP packet while parsing.\n");
		return -1;
	}

	dbgprintf("[TCP] Incoming TCP packet: %d syn, %d ack, %d (%d)\n", hdr->syn, hdr->ack, hdr->seq, htonl(hdr->seq));

	switch (sk->tcp->state){
	case TCP_LISTEN:
		if(hdr->syn == 1 && hdr->ack == 0){
			return tcp_recv_syn(sk, skb);
		}
		break;
	
	case TCP_SYN_RCVD:
		if(hdr->syn == 0 && hdr->ack == 1){
			/* Handle syn / ack */
		}
		break;
	
	case TCP_SYN_SENT:
		if(hdr->syn == 1 && hdr->ack == 1){
			tcp_send_ack(sk, hdr, 1);
			sk->tcp->state = TCP_ESTABLISHED;

			dbgprintf("Socket %d set to established\n", sk);
			return 0;
		}
		break;
	case TCP_WAIT_ACK:
		if(hdr->syn == 0 && hdr->ack == 1){
			dbgprintf("Socket %d received ack for %d\n", sk, hdr->ack_seq);
			tcp_recv_ack(sk, hdr);
			return 0;
		}
		break;
	case TCP_ESTABLISHED:
		if(hdr->syn == 0 && hdr->ack == 1){
			dbgprintf("Socket %d received data for %d\n", sk, hdr->ack_seq);
			//tcp_recv_segment(sk, hdr, skb);

			/* Check if correct packet is in order. */
			int ret = net_sock_add_data(sk, skb);
			tcp_send_ack(sk, hdr, skb->data_len);
			if(ret == 0)
				skb_free(skb);
			return 0;
		}
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

