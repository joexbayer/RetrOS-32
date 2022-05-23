#include <e1000.h>
#include <screen.h>
#include <pci.h>

#define PACKET_SIZE   2048
#define TX_SIZE 32
#define RX_SIZE 128
#define TX_BUFF_SIZE sizeof(e1000_tx_desc_t) * TX_SIZE
#define RX_BUFF_SIZE sizeof(e1000_rx_desc_t) * RX_SIZE

volatile uint32_t *e1000;
#define E1000_DEVICE_SET(offset) (e1000[offset >> 2])
#define E1000_DEVICE_GET(offset) E1000_DEVICE_SET(offset)

uint32_t debug_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};

/* Allocate space for transmit and recieve buffers. */
e1000_tx_desc_t tx_desc_list[TX_SIZE];
char tx_buf[TX_SIZE][PACKET_SIZE];

e1000_rx_desc_t rx_desc_list[RX_SIZE];
char rx_buf[RX_SIZE][PACKET_SIZE];

static void _e1000_reset_tx_desc()
{
	memset(tx_desc_list, 0, TX_BUFF_SIZE);
    for (size_t i = 0; i < TX_SIZE; i++)
    {
	/* Initialize transmit buffers  */
	tx_desc_list[i].buffer_addr = tx_buf[i];
	tx_desc_list[i].status = E1000_TXD_STAT_DD;
	tx_desc_list[i].cmd = E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP;
    }
}

static void _e1000_reset_rx_desc()
{
    memset(rx_desc_list, 0, RX_BUFF_SIZE);
    for (size_t i = 0; i < RX_SIZE; i++)
    {
	/* Initialize recv buffers  */
	rx_desc_list[i].buffer_addr = rx_buf[i];
    }
}

static void _e1000_mac()
{
	uint32_t low = 0, high = 0;
	int i;

	for (i = 0; i < 4; i++) {
		low |= debug_mac[i] << (8 * i);
	}

	for (i = 4; i < 6; i++) {
		high |= debug_mac[i] << (8 * i);
	}

	E1000_DEVICE_SET(E1000_RA) = low;
	e1000[(E1000_RAH_AV >> 2)+1] = high | E1000_RAH_AV;
}

void _e1000_tx_init()
{
    _e1000_reset_tx_desc();

    /* Set TX buffers for E1000 */
    E1000_DEVICE_SET(E1000_TDBAL) = tx_desc_list;
    E1000_DEVICE_SET(E1000_TDBAH) = 0;
    E1000_DEVICE_SET(E1000_TDLEN) = TX_BUFF_SIZE;

    /* tx desc tail and head must be 0 */
    E1000_DEVICE_SET(E1000_TDH) = 0;
    E1000_DEVICE_SET(E1000_TDT) = 0;

    /* Enable TX, for more options check e1000.h */
				   /* enable tx */  /* pad short packets */ /* collision threshold */       /* collision distance */
    E1000_DEVICE_SET(E1000_TCTL) = E1000_TCTL_EN   | E1000_TCTL_PSP         | (E1000_TCTL_CT & (0x10 << 4)) | (E1000_TCTL_COLD & (0x40 << 12));

	/* 13.4.34 Transmit IPG Register
	   This register controls the IPG (Inter Packet Gap) timer for the Ethernet controller.
	*/
    E1000_DEVICE_SET(E1000_TIPG) = 10 | (8 << 10) | (12 << 20);
}

void _e1000_rx_init()
{
	_e1000_reset_rx_desc();

	E1000_DEVICE_SET(E1000_IMS) = 0;
	E1000_DEVICE_SET(E1000_ICS) = 0;

	/* Set RX buffers for E1000 */
	E1000_DEVICE_SET(E1000_RDBAL) = rx_desc_list;
	E1000_DEVICE_SET(E1000_RDBAH) = 0;

	E1000_DEVICE_SET(E1000_RDLEN) = RX_BUFF_SIZE;
	E1000_DEVICE_SET(E1000_RDT) = 128-1;
	E1000_DEVICE_SET(E1000_RDH) = 0;
	
	/* Enable RX, for more options check e1000.h */
								   /* enable */	   /* Strip Ethernet CRC */  /* broadcast enable */  /* rx buffer size 2048 */
	E1000_DEVICE_SET(E1000_RCTL) = E1000_RCTL_EN | E1000_RCTL_SECRC     |    E1000_RCTL_BAM    |     E1000_RCTL_SZ_2048;;	
}

static int next = 0;
static int errors = 0;
int e1000_receive()
{
	int tail = E1000_DEVICE_SET(E1000_RDT);
	if(!(rx_desc_list[next].status & E1000_RXD_STAT_DD)) /* Descriptor Done */
	{
		scrprintf(12, 13, "E1000 INTERRUPT ERROR 0x%x, %x, %d, %d", rx_desc_list[next].status, errors++, tail, next);
		return -1;
	}

	int length = rx_desc_list[next].length;
	if(length >= PACKET_SIZE)
		goto drop;

	char msg[PACKET_SIZE];
	memcpy(msg, rx_buf[next], length);
	scrprintf(12, 14, "E1000 READ PACKET 0x%x, size %d", next, length);

drop:
	rx_desc_list[next].status = 0;
	next = (next + 1) % RX_SIZE;
	E1000_DEVICE_SET(E1000_RDT) = (tail + 1 ) % RX_SIZE;
	return 0;
}

int e1000_transmit(void* buffer, uint16_t size)
{
	if(size >= PACKET_SIZE)
		return -1;

	char* ptr = (char*) buffer;
	uint16_t tail = E1000_DEVICE_GET(E1000_TDT);

	e1000_tx_desc_t* txdesc = &tx_desc_list[tail];
	if(!(txdesc->status & E1000_TXD_STAT_DD)) /* Check if status is DD (Descriptor Done) */
		return -1;

	memcpy(tx_buf[tail], ptr, size);
	txdesc->length = size;
	txdesc->status &= ~E1000_TXD_STAT_DD;
	E1000_DEVICE_SET(E1000_TDT) = (tail+1) % TX_SIZE;

	return 0;
}

static void e1000_callback()
{
    scrwrite(12, 12, "E1000 INTERRUPT", VGA_COLOR_LIGHT_BROWN);
	e1000_receive();

	E1000_DEVICE_GET(E1000_ICR);
}

void e1000_attach(pci_device_t* dev)
{
    scrwrite(50, 13, "Intel E1000:", VGA_COLOR_LIGHT_GREEN);
    scrprintf(50, 14, "Base: 0x%x", dev->base);

    e1000 = dev->base;
    pci_enable_device_busmaster(dev->bus, dev->slot, dev->function);

	_e1000_tx_init();
	scrprintf(50, 15, "Transmit Queue: 0x%x", tx_desc_list);
	_e1000_rx_init();
	scrprintf(50, 16, "Recieve Queue: 0x%x", rx_desc_list);

	_e1000_mac();

    /* For now.. hard code irq to 11 */
    isr_install(32+11, &e1000_callback);

	E1000_DEVICE_SET(E1000_RDTR) = 0;
	E1000_DEVICE_SET(E1000_RADV) = 0;
	E1000_DEVICE_SET(E1000_IMS) = (1 << 7);

}