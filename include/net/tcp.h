#ifndef __TCP_H
#define __TCP_H

#include <stdint.h>
#include <sync.h>
#include <rbuffer.h>

struct tcp_connection {
	uint8_t state;

	uint32_t sequence;
	uint32_t acknowledgement;

	uint32_t retransmits;
	uint32_t tcpi_snd_mss;
	uint32_t tcpi_rcv_mss;

	uint32_t* last_data_sent;
	uint32_t* last_ack_sent;

	uint32_t* last_ack_recv;
	uint32_t* last_data_recv;

	struct ring_buffer* recv_buffer;
	uint8_t data_ready;
	uint32_t recvd;

	/* This identifies a TCP connection. */
	uint16_t dport;
	uint16_t sport;
	uint32_t sip;

	uint16_t backlog;
};

#include <net/socket.h>

#define TCP_MSS        512
#define TCP_MAX_BUFFER_SIZE 4096*4

/* TCP STATES */
enum {
	TCP_CREATED,
	TCP_CLOSED,
	TCP_LISTEN,
	TCP_WAIT_ACK,
	TCP_SYN_RCVD,
	TCP_SYN_SENT,
	/* Between SYN_SENT and ESTABLISHED a new socket is created. */
	TCP_ESTABLISHED,
	TCP_FIN_WAIT_1,
	TCP_FIN_WAIT_2,
	TCP_CLOSING,
	TCP_TIME_WAIT,
	TCP_CLOSE_WAIT,
	TCP_LAST_ACK
};

#define TCP_HTONS(hdr) \
	(hdr)->seq = htonl((hdr)->seq); \
	(hdr)->ack_seq = htonl((hdr)->ack_seq);\
	(hdr)->window = htons((hdr)->window); \
	(hdr)->check = htons((hdr)->check); \
	(hdr)->urg_ptr = htons((hdr)->urg_ptr); \

struct tcp_header
  {
    uint16_t source;
    uint16_t dest;
    uint32_t seq;
    uint32_t ack_seq;
    uint16_t res1:4;
    uint16_t doff:4;
    uint16_t fin:1;
    uint16_t syn:1;
    uint16_t rst:1;
    uint16_t psh:1;
    uint16_t ack:1;
    uint16_t urg:1;
    uint16_t res2:2;

    uint16_t window;
    uint16_t check;
    uint16_t urg_ptr;
};


int tcp_is_listening(struct sock* sock);
int tcp_set_listening(struct sock* sock, int backlog);
int tcp_register_connection(struct sock* sock, uint16_t dst_port, uint16_t src_port);
int tcp_connect(struct sock* sock);
int tcp_send_segment(struct sock* sock, uint8_t* data, uint32_t len, uint8_t push);
int tcp_parse(struct sk_buff* skb);

int tcp_read(struct sock* sock, uint8_t* buffer, unsigned int length);

#endif
