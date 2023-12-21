#ifndef __TCP_H
#define __TCP_H

#include <stdint.h>
#include <util.h>
#include <sync.h>
#include <rbuffer.h>

/* TCP STATES */
typedef enum {
	TCP_CREATED,
	TCP_CLOSED,	    /* represents no connection state at all. */
	TCP_LISTEN,    	/* represents waiting for a connection request from any remote TCP and port. */
	TCP_WAIT_ACK,
	TCP_SYN_RCVD, 	/* represents waiting for a confirming connection request acknowledgment after having both received and sent a connection request. */
	TCP_SYN_SENT, 	/* represents waiting for a matching connection request after having sent a connection request. */
	/* Between SYN_SENT and ESTABLISHED a new socket is created. */
	TCP_ESTABLISHED,/* represents an open connection, data received can be delivered to the user. The normal state for the data transfer phase of the connection. */
	TCP_FIN_WAIT,   /* represents waiting for a connection termination request from the remote TCP, or an acknowledgment of the connection termination request previously sent. */
	TCP_FIN_WAIT_2, /* represents waiting for a connection termination request from the remote TCP. */
	TCP_CLOSING,    /* represents waiting for a connection termination request acknowledgment from the remote TCP. */
	TCP_TIME_WAIT,
	TCP_CLOSE_WAIT,
	TCP_LAST_ACK,   /* represents waiting for an acknowledgment of the connection termination request previously sent to the remote TCP (which includes an acknowledgment of its connection termination request). */
	TCP_PREPARE,
	TCP_CLOSE_WAIT2
	
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

/**
 *   0                   1                   2                   3   
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |          Source Port          |       Destination Port        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        Sequence Number                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                    Acknowledgment Number                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |  Data |           |U|A|P|R|S|F|                               |
 *  | Offset| Reserved  |R|C|S|S|Y|I|            Window             |
 *  |       |           |G|K|H|T|N|N|                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |           Checksum            |         Urgent Pointer        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                    Options                    |    Padding    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                             data                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
 */
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

/**
 * @brief Transmission Control Block
 * The maintenance of a TCP
 * connection requires the remembering of several variables.  We conceive
 * of these variables being stored in a connection record called a
 * Transmission Control Block or TCB.
 */
struct tcb {
	struct sock* sock; /* ? */

	tcp_state_t state;

	uint16_t dport;
	uint16_t sport;
	uint32_t sip;

	struct ring_buffer* rbuf;
	struct ring_buffer* sbuf;

	struct skb_queue* retransmit;
	
	/**
	 * @brief Send Sequence Variables
	 *           1          2          3          4      
     * ----------|----------|----------|---------- 
     * 		  SND.UNA    SND.NXT    SND.UNA        
     * 							   +SND.WND        
	 */
	uint32_t snd_una; 	/* oldest unacknowledged sequence number */
	uint32_t snd_nxt; 	/* next sequence number to be sent */
	uint32_t snd_wnd; 	/* send window */
	uint32_t snd_up; 	/* send urgent pointer */
	uint32_t snd_wl1; 	/* segment sequence number used for last window update */
	uint32_t snd_wl2; 	/* segment acknowledgment number used for last window update */
	uint32_t iss; 		/* initial send sequence number */

	/* Receive Sequence Variables */
	uint32_t rcv_nxt; 	/* next sequence number expected on an incoming segments, and is the left or lower edge of the receive window */
	uint32_t rcv_wnd;   /* receive window */
	uint32_t rcv_up;	/* receive urgent pointer */
	uint32_t irs;		/* initial receive sequence number */

	/* Current Segment Variables */
	uint32_t seg_seq;	/* sequence number of the incoming segment */
	uint32_t seg_ack;	/* acknowledgment number of the incoming segment */
	uint32_t seg_len;	/* length of the incoming segment */
	uint32_t seq_wnd;	/* segment sequence number + segment length */
	uint32_t seq_up;
	uint32_t seq_prc;
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
