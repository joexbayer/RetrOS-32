#include <net/tcp.h>
#include <net/skb.h>

#define MAX_TCP_CONNECTIONS 20
#define TCP_MAX_SIZE 1500 //? 

static struct tcp_connection tcp_connections[MAX_TCP_CONNECTIONS];

int tcp_register_connection(uint16_t dst_port, uint16_t src_port)
{

    return 0;
}


int tcp_send_ack(struct tcp_connection* conn, uint16_t dst_port, uint16_t src_port)
{
  return 0;
}

int tcp_send_syn(struct tcp_connection* conn, uint16_t dst_port, uint16_t src_port)
{

  return 0;
}


int tcp_parse(struct skb_buff* skb)
{
    
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
        break;
      default:
        
        break;
    }
  }
  return;
}

