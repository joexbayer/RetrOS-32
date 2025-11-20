/**
 * @file tcp.c
 * @author Joe Bayer (joexbayer)
 * @brief TCP implementation.
 * @version 0.1
 * @date 2023-12-19
 * 
 * @see https://ietf.org/rfc/rfc793.txt
 * @copyright Copyright (c) 2023
 * 
 */

#include <work.h>
#include <net/tcp.h>
#include <memory.h>
#include <net/skb.h>
#include <net/dhcp.h>
#include <net/net.h>
#include <assert.h>
#include <serial.h>
#include <scheduler.h>
#include <errors.h>
#include <timer.h>

#define TCB_MAX 32

/** new implementation **/
static struct skb_queue* retry_queue = NULL;
static struct tcp_manager {
	struct tcb* tcbs[TCB_MAX];
	int tcb_count;
} tcp_manager = {0};

/* Prototypes */
static int tcp_state_machine(struct sk_buff* skb);

int tcp_init()
{
	/* initialize the TCP manager */
	tcp_manager.tcb_count = 0;

	/* create the retry queue */
	retry_queue = skb_new_queue();
	if(retry_queue == NULL){
		dbgprintf("[TCP] Failed to create retry queue!\n");
		return -1;
	}

	return ERROR_OK;
}

/**
 * @brief Create a new pending connections list for a listening socket
 */
struct tcp_pending_list* tcp_pending_list_create()
{
	struct tcp_pending_list* list = create(struct tcp_pending_list);
	if(list == NULL){
		return NULL;
	}
	
	memset(list, 0, sizeof(struct tcp_pending_list));
	mutex_init(&list->lock);
	list->count = 0;
	
	return list;
}

/**
 * @brief Destroy a pending connections list
 */
void tcp_pending_list_destroy(struct tcp_pending_list* list)
{
	if(list == NULL) return;
	kfree(list);
}

/**
 * @brief Add a new pending connection to the list
 * @return 0 on success, -1 on failure (list full)
 */
int tcp_pending_add(struct tcp_pending_list* list, uint32_t remote_ip, uint16_t remote_port, 
                    uint32_t initial_seq, uint32_t our_seq)
{
	if(list == NULL) return -1;
	
	acquire(&list->lock);
	
	/* Find a free slot */
	for(int i = 0; i < TCP_MAX_PENDING_CONNECTIONS; i++){
		if(!list->connections[i].valid){
			list->connections[i].remote_ip = remote_ip;
			list->connections[i].remote_port = remote_port;
			list->connections[i].initial_seq = initial_seq;
			list->connections[i].our_seq = our_seq;
			list->connections[i].timestamp = timer_get_tick();
			list->connections[i].valid = 1;
			list->count++;
			
			release(&list->lock);
			dbgprintf("[TCP] Added pending connection from %x:%d (count: %d)\n", 
			          ntohl(remote_ip), ntohs(remote_port), list->count);
			return 0;
		}
	}
	
	release(&list->lock);
	dbgprintf("[TCP] Pending connections list is full!\n");
	return -1;
}

/**
 * @brief Find a pending connection by remote IP and port
 * @return Pointer to connection if found, NULL otherwise
 */
struct tcp_pending_connection* tcp_pending_find(struct tcp_pending_list* list, 
                                                 uint32_t remote_ip, uint16_t remote_port)
{
	if(list == NULL) return NULL;
	
	acquire(&list->lock);
	
	for(int i = 0; i < TCP_MAX_PENDING_CONNECTIONS; i++){
		if(list->connections[i].valid && 
		   list->connections[i].remote_ip == remote_ip &&
		   list->connections[i].remote_port == remote_port){
			release(&list->lock);
			return &list->connections[i];
		}
	}
	
	release(&list->lock);
	return NULL;
}

/**
 * @brief Remove a pending connection from the list
 * @return 0 on success, -1 if not found
 */
int tcp_pending_remove(struct tcp_pending_list* list, uint32_t remote_ip, uint16_t remote_port)
{
	if(list == NULL) return -1;
	
	acquire(&list->lock);
	
	for(int i = 0; i < TCP_MAX_PENDING_CONNECTIONS; i++){
		if(list->connections[i].valid && 
		   list->connections[i].remote_ip == remote_ip &&
		   list->connections[i].remote_port == remote_port){
			list->connections[i].valid = 0;
			list->count--;
			release(&list->lock);
			dbgprintf("[TCP] Removed pending connection from %x:%d (count: %d)\n", 
			          ntohl(remote_ip), ntohs(remote_port), list->count);
			return 0;
		}
	}
	
	release(&list->lock);
	return -1;
}

/**
 * @brief Clean up stale pending connections that have timed out
 */
void tcp_pending_cleanup_stale(struct tcp_pending_list* list, uint32_t current_time, uint32_t timeout)
{
	if(list == NULL) return;
	
	acquire(&list->lock);
	
	for(int i = 0; i < TCP_MAX_PENDING_CONNECTIONS; i++){
		if(list->connections[i].valid){
			uint32_t age = current_time - list->connections[i].timestamp;
			if(age > timeout){
				dbgprintf("[TCP] Removing stale pending connection (age: %d)\n", age);
				list->connections[i].valid = 0;
				list->count--;
			}
		}
	}
	
	release(&list->lock);
}

/**
 * @brief Clean up stale pending connections across all listening sockets
 * @param timeout_ticks How old a pending connection must be to be considered stale
 */
void tcp_cleanup_all_pending_connections(uint32_t timeout_ticks)
{
	uint32_t current_time = timer_get_tick();
	struct sockets sockets;
	
	if(net_get_sockets(&sockets) < 0){
		return;
	}
	
	for(int i = 0; i < sockets.total_sockets; i++){
		struct sock* sock = sockets.sockets[i];
		if(sock == NULL || sock->tcp == NULL){
			continue;
		}
		
		if(sock->tcp->state == TCP_LISTEN && sock->pending_connections != NULL){
			tcp_pending_cleanup_stale(sock->pending_connections, current_time, timeout_ticks);
		}
	}
}

int tcp_retry_queue_size(){
	if(retry_queue == NULL){
		warningf("[TCP] Retry queue is not initialized!\n");
		return -1;
	}
	return retry_queue->size;
}

int tcp_retry_all(){

	if(retry_queue == NULL){
		warningf("[TCP] Retry queue is not initialized!\n");
		return -1;
	}

	/* Process at most the current queue size to avoid infinite loops
	 * if packets are re-added to the queue during processing */
	int initial_size = retry_queue->size;
	int processed = 0;
	
	while(processed < initial_size && retry_queue->size > 0){
		struct sk_buff* skb = retry_queue->ops->remove(retry_queue);
		if(skb == NULL){
			dbgprintf("[TCP] Failed to remove from retry queue!\n");
			return -1;
		}

		dbgprintf("[TCP] Retrying packet %d (attempt %d/3)\n", skb->len, skb->retries);
		int ret = tcp_state_machine(skb);
		if(ret < 0){
			skb_free(skb);
		}
		processed++;
	}

	return ERROR_OK;
}

int tcb_init()
{
	memset(&tcp_manager, 0, sizeof(struct tcp_manager));
	return ERROR_OK;
}

/**
 * @brief Creates a new TCB. 
 * Allocates all necessary memory for a new TCB.
 * @return struct tcb*, NULL on failure.
 */
struct tcb* tcb_new()
{
	struct tcb* tcb = NULL;
	if(tcp_manager.tcb_count >= TCB_MAX){
		dbgprintf("[TCP] Max number of TCBs reached!\n");
		goto tcb_new_error;
	}

	tcb = create(struct tcb); 
	if(tcb == NULL){
		dbgprintf("[TCP] Failed to allocate TCB!\n");
		goto tcb_new_error;
	}

	memset(tcb, 0, sizeof(struct tcb));

	tcb->rbuf = rbuffer_new(1024);
	if(tcb->rbuf == NULL){
		dbgprintf("[TCP] Failed to allocate receive buffer!\n");
		goto tcb_new_error;
	}

	tcb->sbuf = rbuffer_new(1024);
	if(tcb->sbuf == NULL){
		dbgprintf("[TCP] Failed to allocate send buffer!\n");
		goto tcb_new_error;
	}

	tcb->retransmit = skb_new_queue();
	if(tcb->retransmit == NULL){
		dbgprintf("[TCP] Failed to allocate retransmit queue!\n");
		goto tcb_new_error;
	}
	/* register in manager */
	tcp_manager.tcbs[tcp_manager.tcb_count++] = tcb;

	tcb->state = TCP_CREATED;

	return tcb;

tcb_new_error:
	if(tcb != NULL) kfree(tcb);
	if(tcb != NULL && tcb->rbuf != NULL) rbuffer_free(tcb->rbuf);
	if(tcb != NULL && tcb->sbuf != NULL) rbuffer_free(tcb->sbuf);
	if(tcb != NULL && tcb->retransmit != NULL) skb_free_queue(tcb->retransmit);
	return NULL;
}


#define IS_TCP_SOCKET(sock) (sock->type == SOCK_STREAM && sock->tcp != NULL)

#define TCP_BLOCK(sock)\
	sock->waiting = $process->current;\
	$process->current->state = BLOCKED;\
	dbgprintf("[TCP] Blocking process %d on socket %d: CLI: %d\n", $process->current->pid, sock->socket, __cli_cnt);\
	kernel_yield();

#define TCP_UNBLOCK(sock)\
	if(sock->waiting != NULL){\
		dbgprintf("[TCP] Unblocking process %d on socket %d\n", sock->waiting->pid, sock->socket);\
		sock->waiting->state = RUNNING;\
		sock->waiting = NULL;\
	}


static const char* tcp_state_str[] = {
	"TCP_CREATED",
	"TCP_CLOSED",
	"TCP_LISTEN",
	"TCP_WAIT_ACK",
	"TCP_SYN_RCVD",
	"TCP_SYN_SENT",
	"TCP_ESTABLISHED",
	"TCP_FIN_WAIT",
	"TCP_FIN_WAIT_2",
	"TCP_CLOSING",
	"TCP_TIME_WAIT",
	"TCP_CLOSE_WAIT",
	"TCP_LAST_ACK",
	"TCP_PREPARE"
};
char* tcp_state_to_str(tcp_state_t state){
	if(state > TCP_PREPARE) return "UNKNOWN";
	if(state < TCP_CREATED) return "UNKNOWN";
	
	return (char*)tcp_state_str[state];
}

/**
 * @brief Creates a new TCP connection.
 * Function allocates memory for a new TCP connection and initializes it.
 * @param sock generic socket to create connection for.
 * @param dst_port destination port.
 * @param src_port source port.
 * @return int 0 on success, -1 on failure.
 */
int tcp_new_connection(struct sock* sock, uint16_t dst_port, uint16_t src_port)
{
	sock->tcp = create(struct tcp_connection);
	ERR_ON_NULL(sock->tcp);

	memset(sock->tcp, 0, sizeof(struct tcp_connection));
	sock->tcp->dport = dst_port;
	sock->tcp->sport = src_port;
	sock->tcp->state = TCP_CREATED;
	sock->tcp->sequence = 1;

	return ERROR_OK;
}

int tcp_free_connection(struct sock* sock)
{
	/* TODO: check for active connections */
	kfree(sock->tcp);
	sock->tcp = NULL;

	return ERROR_OK;
}

inline int tcp_is_listening(struct sock* sock)
{
	return sock->tcp->state == TCP_LISTEN;
}

inline int tcp_set_listening(struct sock* sock, int backlog)
{
	/* create backlog queue, I do not check if there already is a queue... */
	sock->backlog.queue = skb_new_queue();
	ERR_ON_NULL(sock->backlog.queue);

	sock->backlog.size = backlog;
	sock->backlog.count = 0;

	/* Create pending connections list for tracking half-open connections */
	if(sock->pending_connections == NULL){
		sock->pending_connections = tcp_pending_list_create();
		if(sock->pending_connections == NULL){
			dbgprintf("[TCP] Failed to create pending connections list!\n");
			return -1;
		}
	}

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
/**
 * @brief Sends a TCP segment.
 * Function sends given data as a TCP segment.
 * @warning Calls net_send_skb() which frees the SKB.
 * @param sock generic socket to send from.
 * @param hdr TCP header to send.
 * @param skb SKB to send.
 * @param data given data to send.
 * @param len length of data
 * @return int 
 */
static int __tcp_send(struct sock* sock, struct tcp_header* hdr, struct sk_buff* skb, uint8_t* data, uint32_t len)
{
	int ret;

	dbgprintf("[TCP - %d] <- TCP packet: %d syn, %d ack, %d fin %d push (src port: %d, dest port: %d)\n", 
		timer_get_tick(), hdr->syn, hdr->ack, hdr->fin, hdr->psh, htons(hdr->source), htons(hdr->dest));

	if(net_ipv4_add_header(skb, sock->recv_addr.sin_addr.s_addr, TCP, sizeof(struct tcp_header)+len) < 0){
		skb_free(skb);
		return -1;
	}
	TCP_HTONS(hdr);

	memcpy(skb->data, hdr, sizeof(struct tcp_header));
	hdr = (struct tcp_header*) skb->data;

	skb->len += sizeof(struct tcp_header);
	skb->data += sizeof(struct tcp_header);

	if(len > 0){
		memcpy(skb->data, data, len);
		skb->len += len;
		skb->data += len;
	}

	/**
	 * @brief TCP header checksum is calculated over the pseudo header and the TCP header.
	 * This pseudo header contains the Source Address, the Destination Address, the Protocol, and TCP length.
	 */
	hdr->check = tcp_calculate_checksum(skb->hdr.ip->daddr, skb->hdr.ip->saddr, (unsigned short*)hdr, sizeof(struct tcp_header)+len);

	ret = net_send_skb(skb);
	if(ret < 0){
		dbgprintf("[TCP] Failed to send segment\n");
		return -1;
	}

	return ERROR_OK;
}

/**
 * @brief Sends a TCP segment.
 * Function sends given data as a TCP segment. Creates a SKB and fills in the header
 * before sending it to the network daemon.
 * @param sock generic socket to send from.
 * @param data given data to send.
 * @param len length of data
 * @param push if set to 1, the PSH flag will be set.
 * @return int 0 on success, -1 on failure.
 */
int tcp_send_segment(struct sock* sock, uint8_t* data, uint32_t len, uint8_t push)
{
	uint8_t retries;
	uint32_t timeout;
	uint32_t seq = sock->tcp->sequence;
	uint32_t ack = sock->tcp->acknowledgement;
	struct sk_buff* skb;

	dbgprintf("[TCP] Sending segment with size %d, seq: %d (%d after)\n", len, sock->tcp->sequence, sock->tcp->sequence+len);

	sock->tcp->sequence += len;

	/**
	 * @brief This is where we need to check if packet is lost.
	 * @see https://github.com/joexbayer/RetrOS-32/issues/30
	 * TODO: check if packet is lost.
	 * Simples solution is to just wait for an ACK, if we dont get one, resend the packet.
	 * Need a timer to resend the packet.
	 * 
	 * Problem is that we "lose" the skb after it is transmitted, keep a copy?
	 */
	retries = 0;

	do {
		struct tcp_header hdr = {
			.source = sock->bound_port,
			.dest = sock->recv_addr.sin_port,
			.window = 1500,
			.seq = seq,
			.ack_seq = ack,
			.doff = 0x05,
			.ack = 1,
			.psh = push
		};

		skb = skb_new();
		ERR_ON_NULL(skb);

		/* Send segment, __tcp_send consumes skb */
		__tcp_send(sock, &hdr, skb, data, len);

		/* Wait for ACK */
		timeout = timer_get_tick() + 1000;

		/* check if ack was receiver for timeout seconds. */
		while((uint32_t)timer_get_tick() < timeout){
			kernel_yield();
			if(!net_sock_awaiting_ack(sock)) return ERROR_OK;
		}
		dbgprintf("[TCP] Timeout for %d\n", htonl(hdr.seq));
	} while (retries++ < 3);

	dbgprintf("[TCP] Failed to send segment\n");
	sock->tcp->sequence = seq;
	return -1;
}

int tcp_accept_connection(struct sock* sock, struct sock* new)
{
    if(sock->tcp == NULL){
		dbgprintf("[TCP] Socket %d has no TCP state\n", sock);
        return -1;
     }

	/* Allow accept even if socket is in SYN_RCVD (processing new connection) */
	if(sock->tcp->state != TCP_LISTEN && sock->tcp->state != TCP_SYN_RCVD){
		dbgprintf("[TCP] Socket %d is not listening (state: %s)\n", sock, tcp_state_to_str(sock->tcp->state));
		return -1;
	}

    while(sock->backlog.count == 0){
		dbgprintf("[TCP] Socket %d is listening but backlog is empty (waiting=%p)\n", sock, sock->waiting);
		TCP_BLOCK(sock);
	}
	
	dbgprintf("[TCP] Accept proceeding with backlog count=%d\n", sock->backlog.count);

	struct sk_buff* skb = sock->backlog.queue->ops->remove(sock->backlog.queue);
	ERR_ON_NULL(skb);
	sock->backlog.count--;

	struct tcp_header* hdr = (struct tcp_header*) skb->hdr.tcp;

	/**
	 * @brief Extract connection info from the SKB, not from listening socket.
	 * The SKB contains the final ACK which has all the connection details.
	 */
	struct sockaddr_in remote_addr;
	remote_addr.sin_port = hdr->source;
	remote_addr.sin_addr.s_addr = skb->hdr.ip->saddr;
	remote_addr.sin_family = AF_INET;
	
	net_prepare_tcp_sock(new, sock->bound_port, &remote_addr);

	new->tcp->state = TCP_ESTABLISHED;
	new->tcp->acknowledgement = htonl(hdr->seq); /* Client's next seq (already accounts for SYN) */
	new->tcp->sequence = hdr->ack_seq;
	sock->accept_sock = NULL;
	
	/* Listening socket should always remain in LISTEN state */
	sock->tcp->state = TCP_LISTEN;
	
	skb_free(skb);
	
	/* 
	 * Trigger immediate retry processing for this new connection.
	 * Data packets may have arrived before accept() was called and are
	 * sitting in the retry queue waiting for this socket to exist.
	 */
	dbgprintf("[TCP] Processing retry queue after accept for %x:%d\n",
	          ntohl(remote_addr.sin_addr.s_addr), ntohs(remote_addr.sin_port));
	tcp_retry_all();

	return ERROR_OK; 
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
	sock->tcp->acknowledgement = htonl(tcp->seq)+len;

	//dbgprintf("[TCP] Sending ack for %d (seq: %d, ack: %d)\n", htonl(tcp->seq)+len, htonl(tcp->ack_seq), htonl(tcp->seq)+1);

	__tcp_send(sock, &hdr, skb, NULL, 0);
	return ERROR_OK;
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
	return ERROR_OK;
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
	return ERROR_OK;
}

int tcp_recv_ack(struct sock* sock, struct tcp_header* tcp)
{
	dbgprintf("[TCP] Incoming TCP ack expected %d got %d\n", sock->tcp->sequence, htonl(tcp->ack_seq));
	if(sock->tcp->sequence == htonl(tcp->ack_seq)){
		dbgprintf("[TCP] Correct sequence acked\n");
		sock->tcp->acknowledgement = htonl(tcp->seq);
		sock->tcp->state = TCP_ESTABLISHED;
	}
	
	return ERROR_OK;
}

int tcp_recv_syn(struct sock* sock, struct tcp_header* tcp)
{
	int ret;
	struct sk_buff* skb;
	
	if (sock->tcp->state != TCP_LISTEN){
		dbgprintf("[TCP] Socket %d is not listening (state: %s)\n", sock, tcp_state_to_str(sock->tcp->state));
		return -1;
	}
	
	/* Check if we have room for another pending connection */
	if(sock->pending_connections == NULL){
		dbgprintf("[TCP] No pending connections list on listening socket!\n");
		return -1;
	}
	
	if(sock->pending_connections->count >= TCP_MAX_PENDING_CONNECTIONS){
		dbgprintf("[TCP] Pending connections list is full, dropping SYN\n");
		return -1;
	}
	
	/* Calculate our sequence number for the SYN-ACK */
	uint32_t our_seq = sock->tcp->sequence;
	
	/* Prepare SYN-ACK packet */
	struct tcp_header hdr = {
		.source = sock->bound_port,
		.dest = tcp->source,
		.window = 1500,
		.seq = our_seq,
		.ack_seq = htonl(tcp->seq) + 1,
		.doff = 0x05,
		.syn = 1,
		.ack = 1
	};
	
	skb = skb_new();
	ERR_ON_NULL(skb);
	
	ret = __tcp_send(sock, &hdr, skb, NULL, 0);
	if(ret < 0){
		dbgprintf("[TCP] Failed to send syn ack\n");
		return -1;
	}
	
	/* Add to pending connections list - the listening socket stays in TCP_LISTEN */
	ret = tcp_pending_add(sock->pending_connections, 
	                      sock->recv_addr.sin_addr.s_addr,  /* Already set by caller */
	                      tcp->source,
	                      htonl(tcp->seq),
	                      our_seq);
	if(ret < 0){
		dbgprintf("[TCP] Failed to add to pending connections\n");
		return -1;
	}
	
	/* Update socket's sequence number for next connection */
	sock->tcp->sequence += 1;  /* Increment by 1 as the SYN flag consumes a sequence number */

	return ERROR_OK;
}

int tcp_send_rst(struct sock* sock, struct tcp_header* tcp)
{
	struct sk_buff* skb = skb_new();
	assert(skb != NULL);

	struct tcp_header hdr = {
		.source = sock->bound_port,
		.dest = sock->recv_addr.sin_port,
		.window = 1500,
		.seq = sock->tcp->sequence,
		.ack_seq = htonl(tcp->seq)+1,
		.doff = 0x05,
		.rst = 1
	};

	sock->tcp->state = TCP_CLOSED;

	dbgprintf("[TCP] Sending RST for %d\n", sock->socket);

	__tcp_send(sock, &hdr, skb, NULL, 0);
	return ERROR_OK;
}

int tcp_send_fin(struct sock* sock)
{
	struct sk_buff* skb = skb_new();
	assert(skb != NULL);

	struct tcp_header hdr = {
		.source = sock->bound_port,
		.dest = sock->recv_addr.sin_port,
		.window = 1500,
		.seq = sock->tcp->sequence,
		.ack_seq = sock->tcp->acknowledgement,
		.doff = 0x05,
		.ack = 1,
		.fin = 1
	};

	sock->tcp->sequence += 1;

	dbgprintf("[TCP] Sending fin for %d\n", sock->socket);

	__tcp_send(sock, &hdr, skb, NULL, 0);
	return ERROR_OK;
}

int tcp_close_connection(struct sock* sock)
{
	dbgprintf("[TCP] Closing socket %d (current state: %s)\n", sock->socket, tcp_state_to_str(sock->tcp->state));
	
	/* If we're already in CLOSE_WAIT (received client's FIN), 
	 * transition to LAST_ACK - we're sending our FIN in response.
	 * Otherwise, we're initiating the close, so go to FIN_WAIT. */
	if(sock->tcp->state == TCP_CLOSE_WAIT){
		sock->tcp->state = TCP_LAST_ACK;
	} else if(sock->tcp->state == TCP_ESTABLISHED){
		sock->tcp->state = TCP_FIN_WAIT;
	}
	/* If already in a closing state, don't change it */
	
	tcp_send_fin(sock);

	/* Wait for connection to close, with timeout to prevent hanging forever */
	int timeout = 0;
	while(sock->tcp->state != TCP_CLOSED && timeout < 50){
		kernel_yield();
		timeout++;
	}
	
	/* Force close if timeout reached */
	if(sock->tcp->state != TCP_CLOSED){
		dbgprintf("[TCP] Close timeout for socket %d (state: %s), forcing to CLOSED\n", 
			sock->socket, tcp_state_to_str(sock->tcp->state));
		sock->tcp->state = TCP_CLOSED;
	} else {
		dbgprintf("[TCP] Socket %d closed successfully\n", sock->socket);
	}

	return ERROR_OK;
}

/* IMPORTANT: Returning a negative value results in the network handler freeing the skb! Else we need to do it. */
int tcp_parse(struct sk_buff* skb)
{
	/* Look if there is an active TCP connection, if not look for accept. */

	struct tcp_header* hdr = (struct tcp_header* ) skb->data;
	skb->hdr.tcp = hdr;
	skb->data += hdr->doff*4;
	skb->data_len = skb->hdr.ip->len - skb->hdr.ip->ihl*4 - hdr->doff*4;

	return tcp_state_machine(skb);
}

int print_socks(){
	struct sockets socks;
	net_get_sockets(&socks);

	dbgprintf("%s)\n", socket_type_to_str(SOCK_STREAM));
	for (int i = 0; i < socks.total_sockets; i++){
		struct sock* sock = socks.sockets[i];
		if(sock == NULL || sock->bound_port == 0 || sock->type == SOCK_DGRAM) continue;

		dbgprintf(" %i:%d %i:%d %s  tx: %d  rx: %d\n\n", sock->bound_ip == 1 ? 0 : sock->bound_ip, ntohs(sock->bound_port), ntohl(sock->recv_addr.sin_addr.s_addr), ntohs(sock->recv_addr.sin_port), sock->tcp ? tcp_state_to_str(sock->tcp->state) : "", sock->tx, sock->rx);
	}
}

static int tcp_state_machine(struct sk_buff* skb){
	struct tcp_header* hdr = (struct tcp_header* ) skb->hdr.tcp;

	struct sock* sk = net_sock_find_tcp(hdr->source, hdr->dest, htonl(skb->hdr.ip->saddr));
	if(sk == NULL){
		dbgprintf("[TCP] No socket found for TCP packet while parsing.\n");
		return -1;
	}

	sock_ref(sk);
	int result = -1;

	dbgprintf("[TCP - %d] %s -> TCP packet: %d syn, %d ack, %d fin %d push %d rst (src port: %d, dest port: %d) %d bytes\n", 
		timer_get_tick(), tcp_state_to_str(sk->tcp->state), hdr->syn, hdr->ack, hdr->fin, hdr->psh, hdr->rst, htons(hdr->source), htons(hdr->dest), skb->data_len);

	//print_socks();

	switch (sk->tcp->state){
	case TCP_LISTEN:
		if(hdr->syn == 1 && hdr->ack == 0){
			if(sk->backlog.count == sk->backlog.size){
				dbgprintf("[TCP] Backlog is full, dropping packet\n");
				result = -1;
				goto out;
			}

			/**
			 * @brief Store the remote address in recv_addr of
			 * listening socket. This will be overwritten for each
			 * new accepted socket.
			 * This is techinically bad as we access IP in TCP.
			 */
			sk->recv_addr.sin_port = hdr->source;
			sk->recv_addr.sin_addr.s_addr = skb->hdr.ip->saddr;

			if(tcp_recv_syn(sk, hdr) < 0){
				// NOOP
			}

			skb_free(skb);
			return ERROR_OK;
		}

		/**
		 * @brief Data packets can arrive before the application accepts the
		 * connection. Since the new socket does not yet exist, stash the SKB
		 * in the retry queue so it can be replayed once the accept completes.
		 * Send an ACK immediately to prevent peer from retransmitting.
		 * 
		 * Only retry a few times - if accept() still hasn't been called after
		 * a reasonable number of attempts, the application is likely hung.
		 * Drop the packet to avoid infinite retry loops that waste CPU.
		 */
		if(hdr->ack == 1 && skb->data_len > 0 && skb->retries < 10){
			/* Send ACK on first retry to stop peer retransmission */
			if(skb->retries == 0){
				tcp_send_ack(sk, hdr, skb->data_len);
			}
			
			skb->retries++;
			
			/* Only add back to retry queue if this is a recent retry */
			if(skb->retries <= 5 || (skb->retries % 10 == 0)){
				dbgprintf("[TCP] Adding to retry queue (retry %d/10)\n", skb->retries);
				if(retry_queue->ops->add(retry_queue, skb) < 0){
					dbgprintf("[TCP] Failed to add to retry queue\n");
					result = -1;
					goto out;
				}
				result = ERROR_OK;
				goto out;
			} else {
				/* Skip this retry iteration to slow down retries */
				dbgprintf("[TCP] Skipping retry iteration %d for backpressure\n", skb->retries);
				skb_free(skb);
				result = ERROR_OK;
				goto out;
			}
		}
		
		/* Drop packet if retry limit exceeded or connection not in backlog */
		if(hdr->ack == 1 && skb->data_len > 0 && skb->retries >= 10){
			dbgprintf("[TCP] Dropping packet after %d retries - accept() not called\n", skb->retries);
			result = -1;
			goto out;
		}

		/**
		 * @brief Handle final ACK of 3-way handshake.
		 * This is the ACK that completes the handshake after we sent SYN-ACK.
		 * Verify it matches a pending connection, then add to backlog.
		 */
		if(hdr->syn == 0 && hdr->ack == 1 && skb->data_len == 0){
			/* Look up this connection in pending list */
			struct tcp_pending_connection* pending = tcp_pending_find(
				sk->pending_connections,
				skb->hdr.ip->saddr,
				hdr->source
			);
			
			if(pending == NULL){
				dbgprintf("[TCP] Received ACK for unknown connection from %x:%d\n",
				          ntohl(skb->hdr.ip->saddr), ntohs(hdr->source));
				result = -1;
				goto out;
			}
			
			/* Verify ACK number matches our SYN-ACK sequence + 1 */
			if(htonl(hdr->ack_seq) != pending->our_seq + 1){
				dbgprintf("[TCP] ACK sequence mismatch: expected %u, got %u\n",
				          pending->our_seq + 1, htonl(hdr->ack_seq));
				result = -1;
				goto out;
			}
			
			/* Connection is validated - add to backlog */
			if(sk->backlog.count < sk->backlog.size){
				/* Store remote address info in SKB for accept() to use */
				sk->recv_addr.sin_port = hdr->source;
				sk->recv_addr.sin_addr.s_addr = skb->hdr.ip->saddr;
				
				sk->backlog.queue->ops->add(sk->backlog.queue, skb);
				sk->backlog.count++;
				dbgprintf("[TCP] Connection from %x:%d ready for accept (backlog: %d)\n",
				          ntohl(skb->hdr.ip->saddr), ntohs(hdr->source), sk->backlog.count);
				
				/* Remove from pending list */
				tcp_pending_remove(sk->pending_connections, skb->hdr.ip->saddr, hdr->source);
				
				TCP_UNBLOCK(sk);
				result = ERROR_OK;
				goto out; /* Don't free SKB, it's in backlog */
			} else{
				dbgprintf("[TCP] Backlog is full, dropping ACK\n");
				/* Remove from pending list since we can't accept it */
				tcp_pending_remove(sk->pending_connections, skb->hdr.ip->saddr, hdr->source);
				result = -1;
				goto out;
			}
		}

		

		

		break;
	case TCP_SYN_RCVD:
		if(hdr->rst == 1){
			dbgprintf("[TCP] Received RST packet, closing connection\n");
			sk->tcp->state = TCP_CLOSED;
			sk->data_ready = -1;

			if(sk->waiting != NULL){
				sk->waiting->state = RUNNING;
				sk->waiting = NULL;
			}
			skb_free(skb);
			result = ERROR_OK;
			goto out;
		}

		if(hdr->syn == 0 && hdr->ack == 1){
			/**
			 * @brief Add to backlog 
			 */
			if(sk->backlog.count < sk->backlog.size){
				sk->backlog.queue->ops->add(sk->backlog.queue, skb);
				sk->backlog.count++;
				dbgprintf("[TCP] Added to backlog %d\n", sk->backlog.count);
			} else{
				dbgprintf("[TCP] Backlog is full, dropping packet\n");
				result = -1;
				goto out;
			}

			sk->tcp->state = TCP_LISTEN;
			TCP_UNBLOCK(sk);

			/* Note: We dont free SKB because its added to the backlog */
			result = ERROR_OK;
			goto out;
		}
		break;
	
	case TCP_SYN_SENT:
		if(hdr->syn == 1 && hdr->ack == 1){
			tcp_send_ack(sk, hdr, 1);
			sk->tcp->state = TCP_ESTABLISHED;

			dbgprintf("Socket %d set to established\n", sk);
			skb_free(skb);
			result = ERROR_OK;
			goto out;
		}
		break;
	case TCP_WAIT_ACK:
		if(hdr->rst == 1){
			dbgprintf("[TCP] Received RST packet, closing connection\n");
			sk->tcp->state = TCP_CLOSED;
			sk->data_ready = -1;

			if(sk->waiting != NULL){
				sk->waiting->state = RUNNING;
				sk->waiting = NULL;
			}
			skb_free(skb);
			result = ERROR_OK;
			goto out;
		}

		if(hdr->syn == 0 && hdr->ack == 1){
			
			/* check if i get a already acked retransmit */
			if(sk->tcp->sequence > htonl(hdr->ack_seq)){
				dbgprintf("[TCP] Received ack for already acked packet %d - %d\n", htonl(hdr->ack_seq), sk->tcp->acknowledgement);
				skb_free(skb);
				result = ERROR_OK;
				goto out;
			}

			tcp_recv_ack(sk, hdr);
			skb_free(skb);
			result = ERROR_OK;
			goto out;
		}
		break;
	case TCP_ESTABLISHED:
		if(hdr->rst == 1){
			dbgprintf("[TCP] Received RST packet, closing connection\n");
			sk->tcp->state = TCP_CLOSED;
			sk->data_ready = -1;
			if(sk->waiting != NULL){
				sk->waiting->state = RUNNING;
				sk->waiting = NULL;
			}
			skb_free(skb);
			result = ERROR_OK;
			goto out;
		}

		if(hdr->syn == 0 && hdr->ack == 1 && hdr->fin == 0){
			/**
			 * @brief This is where we should check if the packet is in order.
			 * @see https://github.com/joexbayer/RetrOS-32/issues/34
			 * TODO: check if packet is in order.
			 * Probably need to check if the sequence number is equal to the expected sequence number.
			 * If not, we should probably drop the packet and wait for the correct one. 
			 * (We should probably also send a NACK to the sender)
			 * 
			 * We will not buffer packets for now, but we should probably do that in the future.
			 * sock->tcp->sequence == htonl(tcp->seq)
			 */
			if (sk->tcp->acknowledgement != htonl(hdr->seq)) {
				dbgprintf("[TCP] Out-of-order packet received. Expected seq: %d, received seq: %d\n",sk->tcp->acknowledgement, htonl(hdr->seq));
				result = -1;
				goto out;
			}

			tcp_send_ack(sk, hdr, skb->data_len);

			/* If ret is < 0, then it has been added to the socket skb_queue, therefor we do not want to free it. */
			int ret = net_sock_add_data(sk, skb);
			if(ret == 0){
				skb_free(skb);
			}

			result = ERROR_OK;
			goto out;
		}

		if(hdr->fin == 1 && hdr->ack == 1){
			/**
			 * @brief Only accept FIN if we've received all data in order.
			 * If the FIN arrives before we've processed all data packets,
			 * ignore it - the client will retransmit.
			 * 
			 * FIN packets can arrive with or without data. If it has data,
			 * we need to account for that in the sequence check.
			 */
			uint32_t expected_fin_seq = sk->tcp->acknowledgement + skb->data_len;
			if (expected_fin_seq != htonl(hdr->seq)) {
				dbgprintf("[TCP] Ignoring FIN - data gap detected. Expected seq: %d, FIN seq: %d (data_len: %d)\n",
					expected_fin_seq, htonl(hdr->seq), skb->data_len);
				skb_free(skb);
				result = ERROR_OK;
				goto out;
			}

			/* If FIN came with data, process it first */
			if(skb->data_len > 0){
				tcp_send_ack(sk, hdr, skb->data_len);
				int ret = net_sock_add_data(sk, skb);
				if(ret == 0){
					skb_free(skb);
				}
			} else {
				skb_free(skb);
			}
			
			/* ACK the FIN */
			tcp_send_ack(sk, hdr, 1);
			
			/**
			 * @brief Transition to CLOSE_WAIT state, indicating we received
			 * the client's FIN. The application will call close() which will
			 * send our FIN and complete the shutdown sequence.
			 * 
			 * Don't send FIN here - let the application's close() handle it.
			 */
			sk->tcp->state = TCP_CLOSE_WAIT;
			
			result = ERROR_OK;
			goto out;
		}
		break;
	case TCP_FIN_WAIT:
		if(hdr->fin == 1 && hdr->ack == 1){
			/* Connection succesfully closed */
			tcp_send_ack(sk, hdr, 1);
			sk->tcp->state = TCP_CLOSED;
			dbgprintf("[TCP] Socket %d closed\n", sk->socket);
		}
		break;
	case TCP_CLOSE_WAIT:
		if(hdr->fin == 1){
			/* Duplicate FIN from peer - ACK it again to stop retransmissions */
			tcp_send_ack(sk, hdr, 1);
			skb_free(skb);
			result = ERROR_OK;
			goto out;
		}
		/* Stay in CLOSE_WAIT until application calls close() */
		skb_free(skb);
		result = ERROR_OK;
		goto out;
	case TCP_LAST_ACK:
		/* Waiting for ACK of our FIN after receiving client's FIN */
		if(hdr->fin == 1){
			/* Duplicate FIN from peer - ACK it again */
			tcp_send_ack(sk, hdr, 1);
			skb_free(skb);
			result = ERROR_OK;
			goto out;
		}
		if(hdr->ack == 1){	
			/* Received ACK for our FIN - connection fully closed */
			sk->tcp->state = TCP_CLOSED;
			dbgprintf("[TCP] Socket %d closed (LAST_ACK -> CLOSED)\n", sk->socket);
			skb_free(skb);
			result = ERROR_OK;
			goto out;
		}
		break;	
	default:
		break;
	}
	
out:
	sock_deref(sk);
	return result;
}
