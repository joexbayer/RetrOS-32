/**
 * @file networking.c
 * @author Joe Bayer (joexbayer)
 * @brief Main process for handling all networking traffic. 
 * @version 0.1
 * @date 2022-06-04
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <process.h>

#include <net/netdev.h>
#include <net/packet.h>

#define MAX_OPEN_PORTS 45
#define MAX_QUEUE_SIZE 20

static struct packet queue[MAX_QUEUE_SIZE];

static uint16_t packets = 0;
static uint32_t ip;
static uint16_t ports[MAX_OPEN_PORTS];

int add_queue(uint8_t* buffer, uint16_t size);
int get_next_queue();

void list_net_devices()
{

}

/**
 * @brief Main networking event loop.
 * 
 */
void main()
{

}

PROGRAM(networking, &main)
ATTACH("lsnet", &list_net_devices)
PROGRAM_END