#ifndef __TCP_H
#define __TCP_H

#include <stdint.h>

/* TCP STATES */
enum {
  TCP_CLOSED,
  TCP_LISTEN,
  TCP_SYN_RCVD,
  TCP_SYN_SENT,
  TCP_ESTABLISHED,
  TCP_FIN_WAIT_1,
  TCP_FIN_WAIT_2,
  TCP_CLOSING,
  TCP_TIME_WAIT,
  TCP_CLOSE_WAIT,
  TCP_LAST_ACK
};

struct tcp_header {
   uint16_t src_port;
   uint16_t dst_port;

   uint32_t seq;
   uint32_t ack;

   uint16_t flags;
   uint16_t cwnd;
   uint16_t csum;

   uint16_t urgptr;
    

};

struct tcp_socket {
  

};

#endif
