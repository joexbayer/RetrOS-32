/**
 * @file e1000.c
 * @author Joe Bayer (joexbayer)
 * @brief E1000 Intel Network Cards driver code. 
 * @version 0.1
 * @date 2022-06-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <e1000.h>
#include <arch/interrupts.h>
#include <pci.h>
#include <net/net.h>
#include <net/netdev.h>
#include <memory.h>
#include <serial.h>
#include <kutils.h>
#include <timer.h>

#undef dbgprintf

#define PACKET_SIZE   2048
#define TX_SIZE 16
#define RX_SIZE 16
#define TX_BUFF_SIZE (sizeof(struct e1000_tx_desc) * TX_SIZE)
#define RX_BUFF_SIZE (sizeof(struct e1000_rx_desc) * RX_SIZE)

static volatile uint32_t *e1000;
#define E1000_DEVICE_SET(offset) (e1000[offset >> 2])
#define E1000_DEVICE_GET(offset) E1000_DEVICE_SET(offset)

uint8_t mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};

/* Allocate space for transmit and recieve buffers. */
static struct e1000_tx_desc tx_desc_list[TX_SIZE];
static char* tx_buf[TX_SIZE];

static struct e1000_rx_desc rx_desc_list[RX_SIZE];
static char* rx_buf[RX_SIZE];

static int interrupts = 0;

struct netdev e1000_netdev;

/**
 * @brief Clears the transmit buffers for the e1000
 */
static void _e1000_reset_tx_desc()
{
	memset(tx_desc_list, 0, TX_BUFF_SIZE);
    for (int i = 0; i < TX_SIZE; i++)
    { 
		/* Initialize transmit buffers  */
		tx_desc_list[i].buffer_addr = (uint32_t)tx_buf[i];
		tx_desc_list[i].status  = E1000_TXD_STAT_DD;
		tx_desc_list[i].cmd = (E1000_TXD_CMD_RS >> 24) | (E1000_TXD_CMD_EOP >> 24);
    }
}
/**
 * @brief Clears the recieve buffers for the e1000
 */
static void _e1000_reset_rx_desc()
{
    memset(rx_desc_list, 0, RX_BUFF_SIZE);
    for (int i = 0; i < RX_SIZE; i++)
    {
		/* Initialize recv buffers  */
		rx_desc_list[i].buffer_addr = (uint32_t)rx_buf[i];
    }
}
/**
 * @brief Reads in the debug_mac into correct registers.
 */
static void _e1000_mac()
{
	uint32_t low = 0, high = 0;
	int i;

	for (i = 0; i < 4; i++) {
		low |= mac[i] << (8 * i);
	}

	for (i = 4; i < 6; i++) {
		high |= mac[i] << (8 * i);
	}

	E1000_DEVICE_SET(E1000_RA) = low;
	e1000[(E1000_RAH_AV >> 2)+1] = high | E1000_RAH_AV;
}

/**
 * @brief Initializes the e1000 transmit registers
 * setting control options and giving pointers to transmit buffers.
 */
void _e1000_tx_init()
{
    _e1000_reset_tx_desc();

    /* Set TX buffers for E1000 */
    E1000_DEVICE_SET(E1000_TDBAL) = (uint32_t)tx_desc_list;
    E1000_DEVICE_SET(E1000_TDBAH) = 0;
    E1000_DEVICE_SET(E1000_TDLEN) = TX_BUFF_SIZE;

    /* tx desc tail and head must be 0 */
    E1000_DEVICE_SET(E1000_TDH) = 0;
    E1000_DEVICE_SET(E1000_TDT) = 0;

    /* Enable TX, for more options check e1000.h */
				   /* enable tx */  /* pad short packets */ /* collision threshold */       /* collision distance */
    E1000_DEVICE_SET(E1000_TCTL) |= (E1000_TCTL_EN | E1000_TCTL_PSP |	(E1000_TCTL_CT & (0x10 << 4)) | (E1000_TCTL_COLD & (0x40 << 12))); 

	/* 13.4.34 Transmit IPG Register
	   This register controls the IPG (Inter Packet Gap) timer for the Ethernet controller.
	*/
    E1000_DEVICE_SET(E1000_TIPG) |= (10) | (4 << 10) | (6 << 20);
}

/**
 * @brief Initializes the e1000 receive registers
 * setting control options and giving pointers to receive buffers.
 */
void _e1000_rx_init()
{
	_e1000_reset_rx_desc();

	E1000_DEVICE_SET(E1000_IMS) = 0;
	E1000_DEVICE_SET(E1000_ICS) = 0;

	/* Set RX buffers for E1000 */
	E1000_DEVICE_SET(E1000_RDBAL) = (uint32_t)rx_desc_list;
	E1000_DEVICE_SET(E1000_RDBAH) = 0;

	E1000_DEVICE_SET(E1000_RDLEN) = RX_BUFF_SIZE;
	E1000_DEVICE_SET(E1000_RDT) = RX_SIZE - 1;
	E1000_DEVICE_SET(E1000_RDH) = 0;
	
	/* Enable RX, for more options check e1000.h */
								   /* enable */	   /* Strip Ethernet CRC */  /* broadcast enable */  /* rx buffer size 2048 */
	E1000_DEVICE_SET(E1000_RCTL) = E1000_RCTL_EN | E1000_RCTL_SECRC     |    E1000_RCTL_BAM    |     E1000_RCTL_SZ_2048;;	
}

static int next = 0;
int e1000_receive(char* buffer, uint32_t size)
{
	int tail = E1000_DEVICE_GET(E1000_RDT);
	if(!(rx_desc_list[next].status & E1000_RXD_STAT_DD)) /* Descriptor Done */
	{
		warningf("[e1000 RX] RXD_STAT_DD not set!\n");
		return -1;
	}

	uint32_t length = rx_desc_list[next].length;
	if(length >= PACKET_SIZE || length > size)
	{
		warningf("[e1000 RX] Dropping packet with length %d\n", length);
		length = -1;
		goto drop;
	}

	dbgprintf("[e1000 - %d] Received %d bytes %d!\n",timer_get_tick(), length, __cli_cnt);

	memcpy(buffer, rx_buf[next], length);

drop:
	rx_desc_list[next].status = 0;
	next = (next + 1) % RX_SIZE;
	dbgprintf("[e1000] RXD_STAT_DD set %d (old: %d)!\n", next, (tail + 1 ) % RX_SIZE);
	E1000_DEVICE_SET(E1000_RDT) = (tail + 1 ) % RX_SIZE;
	//dbgprintf("[e1000] received %d bytes! (tail: %d) (\n", length, next);
	return length;
}

/**
 * @brief Put data into correct transmit buffer for e1000
 * card to transmit. Updates tail pointer.
 * 
 * @param buffer data to transmit
 * @param size of data to transmit
 * @return int size of data, returns -1 on error.
 */
int e1000_transmit(char* buffer, uint32_t size)
{
	if(size >= PACKET_SIZE){
		warningf("[e1000] Size %d is too large!\n", size);
		return -1;
	}

	char* ptr = (char*) buffer;
	uint16_t tail = E1000_DEVICE_GET(E1000_TDT);

	struct e1000_tx_desc* txdesc = &tx_desc_list[tail];
	if(!(txdesc->status & E1000_TXD_STAT_DD)){
		warningf("[e1000] DD status is not done!\n");
		return -1;
	} /* Check if status is DD (Descriptor Done) */

	memcpy(tx_buf[tail], ptr, size);
	txdesc->length = size;
	txdesc->status &= ~E1000_TXD_STAT_DD;
	txdesc->cmd |= E1000_TXD_CMD_EOP;

	E1000_DEVICE_SET(E1000_TDT) = (tail+1) % TX_SIZE;

	//dbgprintf("[e1000 - %d] Sending %d bytes! (tail: %d) %d\n", timer_get_tick(), size, (tail + 1) % TX_SIZE, __cli_cnt);
	return size;
}

void __int_handler e1000_callback()
{
	interrupts++;
	E1000_DEVICE_GET(E1000_ICR);
	net_incoming_packet(&e1000_netdev);
}

void e1000_attach(struct pci_device* dev)
{
	
    e1000 = (volatile uint32_t *)dev->base;

	vmem_map_driver_region((uint32_t) e1000, 5);

    pci_enable_device_busmaster(dev->bus, dev->slot, dev->function);

	for (int i = 0; i < TX_SIZE; i++)
		tx_buf[i] = palloc(PACKET_SIZE);
	
	for (int i = 0; i < RX_SIZE; i++)
		rx_buf[i] = palloc(PACKET_SIZE);

	_e1000_tx_init();
	_e1000_rx_init();

	_e1000_mac();

    /* For now.. hard code irq to 11 */
    interrupt_install_handler(32+dev->irq, &e1000_callback);

	E1000_DEVICE_SET(E1000_RDTR) = 0;
	E1000_DEVICE_SET(E1000_RADV) = 0;
	E1000_DEVICE_SET(E1000_IMS) = (1 << 7);

	e1000_netdev = (struct netdev) {
		.name = "E1000",
		.driver = *dev,
		.read = &e1000_receive,
		.write = &e1000_transmit,
		.sent = 0,
		.received = 0,
		.dropped = 0
	};
	memcpy(e1000_netdev.mac, &mac, 6);

	/* very ugly temporary fix */
	current_netdev = e1000_netdev;

	net_register_netdev("eth0", &e1000_netdev);

	/* Attach as current Netdevice. */
	//netdev_attach_driver(dev, &e1000_receive, &e1000_transmit, "Intel E1000", (uint8_t*)&mac);

	dbgprintf("[E1000] Network card Intel E1000 found and attached!.\n");
}