#include <net/tcp.h>
#include <net/skb.h>

#define MAX_TCP_CONNECTIONS 20

static struct tcp_connection tcp_connections[MAX_TCP_CONNECTIONS];

int tcp_register_connection(struct sock* sock, uint16_t dst_port, uint16_t src_port)
{

	return 0;
}

inline int tcp_is_listening(struct sock* sock)
{
	return sock->tcp_conn != NULL && sock->tcp_conn->state == TCP_LISTEN;
}

inline int tcp_set_listening(struct sock* sock, int backlog)
{
	if(sock->tcp_conn != NULL)
		return -1;
	
	sock->tcp_conn->backlog = backlog;
	sock->tcp_conn->state = TCP_LISTEN;

	return 1;
}



int tcp_send_ack(struct tcp_connection* conn, uint16_t dst_port, uint16_t src_port)
{
	return 0;
}

int tcp_send_syn(struct tcp_connection* conn, uint16_t dst_port, uint16_t src_port)
{	

	struct sock* sock = (struct sock*) container_of(&conn, struct sock, tcp_conn);




	return 0;
}

int tcp_recv_syn(struct tcp_connection* conn)
{
	conn->state = TCP_SYN_RCVD;
	return 1;
}


int tcp_parse(struct sk_buff* skb)
{
		struct tcp_header* hdr = (struct udp_header* ) skb->data;
		skb->hdr.tcp = hdr;

		if(hdr->syn == 1 && hdr->ack == 0){
			/* SYN */
		}

		if(hdr->syn == 1 && hdr->ack == 1){
			/* SYN / ACK */
		}

		if(hdr->syn == 0 && hdr->ack == 1){
			/* ACK */
		}
		
		return 0;
}

void tcp_connection_update()
{
	/* This will be a BIG switch statement, executing functions
	* based on state of a tcp connection
	*/


	for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
			
		struct tcp_connection* connection = &tcp_connections[i];

		switch (connection->state) {
			case TCP_CLOSED:
				break;

			case TCP_LISTEN:
				break;
			
			case TCP_SYN_RCVD:
				/* Send syn/ack */
				break;
			default:
				
				break;
		}
	}
	return;
}

