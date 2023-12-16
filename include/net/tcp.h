#ifndef __TCP_H
#define __TCP_H

#include <stdint.h>
#include <util.h>
#include <sync.h>
#include <rbuffer.h>

/* TCP STATES */
typedef enum {
	TCP_CREATED,
	TCP_CLOSED,
	TCP_LISTEN,
	TCP_WAIT_ACK,
	TCP_SYN_RCVD,
	TCP_SYN_SENT,
	/* Between SYN_SENT and ESTABLISHED a new socket is created. */
	TCP_ESTABLISHED,
	TCP_FIN_WAIT,
	TCP_FIN_WAIT_2,
	TCP_CLOSING,
	TCP_TIME_WAIT,
	TCP_CLOSE_WAIT,
	TCP_LAST_ACK,
	TCP_PREPARE
} tcp_state_t;

struct tcp_connection {
	volatile tcp_state_t state;

	uint32_t sequence;
	uint32_t acknowledgement;

	uint32_t retransmits;
	uint32_t tcpi_snd_mss;
	uint32_t tcpi_rcv_mss;

	uint32_t* last_data_sent;
	uint32_t* last_ack_sent;

	uint32_t* last_ack_recv;
	uint32_t* last_data_recv;

	/* This identifies a TCP connection. */
	uint16_t dport;
	uint16_t sport;
	uint32_t sip;

	uint16_t backlog;
};

#include <net/socket.h>

#define TCP_MSS        512


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

char* tcp_state_to_str(tcp_state_t state);
int tcp_is_listening(struct sock* sock);
int tcp_set_listening(struct sock* sock, int backlog);

int tcp_new_connection(struct sock* sock, uint16_t dst_port, uint16_t src_port);
int tcp_free_connection(struct sock* sock);

int tcp_connect(struct sock* sock);
int tcp_send_segment(struct sock* sock, uint8_t* data, uint32_t len, uint8_t push);
int tcp_parse(struct sk_buff* skb);

int tcp_read(struct sock* sock, uint8_t* buffer, unsigned int length);

int tcp_accept_connection(struct sock* sock, struct sock* new);
int tcp_close_connection(struct sock* sock);

#endif
