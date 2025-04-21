#ifndef E1000_H
#define E1000_H

#include <stdint.h>
#include <pci.h>

#define E1000_VENDOR_ID 0x8086
#define E1000_DEVICE_ID 0x100E

/*
    E1000 Device Driver, information from:
    PCI/PCI-X Family of Gigabit Ethernet Controllers Software Developer’s Manual
    82540EP/EM, 82541xx, 82544GC/EI, 82545GM/EM, 82546GB/EB, and 82547xx
*/


/* Definitions from https://github.com/qemu/qemu/blob/master/hw/net/e1000_regs.h */

/* Descriptor Done */
#define E1000_TXD_STAT_DD    0x00000001
/* Report Status */
#define E1000_TXD_CMD_RS     0x08000000
/* End of Packet */
#define E1000_TXD_CMD_EOP    0x01000000

/* Offsets for tx in e1000 */
#define E1000_TDBAL    0x03800  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH    0x03804  /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN    0x03808  /* TX Descriptor Length - RW */

#define E1000_TDT      0x03818  /* TX Descriptor Tail - RW */
#define E1000_TDH      0x03810  /* TX Descriptor Head - RW */

#define E1000_TCTL     0x00400  /* TX Control - RW */

/* Transmit Control */
#define E1000_TCTL_RST    0x00000001    /* software reset */
#define E1000_TCTL_EN     0x00000002    /* enable tx */
#define E1000_TCTL_BCE    0x00000004    /* busy check enable */
#define E1000_TCTL_PSP    0x00000008    /* pad short packets */
#define E1000_TCTL_CT     0x00000ff0    /* collision threshold */
#define E1000_TCTL_COLD   0x003ff000    /* collision distance */
#define E1000_TCTL_SWXOFF 0x00400000    /* SW Xoff transmission */
#define E1000_TCTL_PBE    0x00800000    /* Packet Burst Enable */
#define E1000_TCTL_RTLC   0x01000000    /* Re-transmit on late collision */
#define E1000_TCTL_NRTU   0x02000000    /* No Re-transmit on underrun */
#define E1000_TCTL_MULR   0x10000000    /* Multiple request support */

#define E1000_TIPG     0x00410  /* TX Inter-packet gap -RW */

/* Recieve */
#define E1000_ICS      0x000C8  /* Interrupt Cause Set - WO */
#define E1000_IMS      0x000D0  /* Interrupt Mask Set - RW */
#define E1000_RDBAL    0x02800  /* RX Descriptor Base Address Low - RW */
#define E1000_RDBAH    0x02804  /* RX Descriptor Base Address High - RW */
#define E1000_RDLEN    0x02808  /* RX Descriptor Length - RW */
#define E1000_RDT      0x02818  /* RX Descriptor Tail - RW */
#define E1000_RDH      0x02810  /* RX Descriptor Head - RW */

#define E1000_RCTL     0x00100  /* RX Control - RW */
#define E1000_RCTL_EN  0x00000002    /* enable */
#define E1000_RCTL_SECRC 0x04000000    /* Strip Ethernet CRC */
#define E1000_RCTL_BAM            0x00008000    /* broadcast enable */
#define E1000_RCTL_SZ_2048        0x00000000    /* rx buffer size 2048 */

#define E1000_RA       0x05400  /* Receive Address - RW Array */
#define E1000_RAH_AV  0x80000000        /* Receive descriptor valid */ 

#define E1000_RDTR     0x02820  /* RX Delay Timer - RW */
#define E1000_RADV     0x0282C  /* RX Interrupt Absolute Delay Timer - RW */

#define E1000_RXD_STAT_DD       0x01    /* Descriptor Done */
#define E1000_ICR      0x000C0	/* Interrupt Cause Read - R/clr */

#define E1000_IMS_RXT0    (1 << 7)  // Receive timer interrupt (Receive timer timeout)
#define E1000_IMS_TXDW    (1 << 0)  // Transmit descriptor written back
#define E1000_IMS_RXDMT0  (1 << 4)  // Receive descriptor minimum threshold
#define E1000_IMS_RXO     (1 << 6)  // Receive overrun
#define E1000_IMS_RXDW    (1 << 7)  // Receive descriptor write-back


/* transmit descriptor */
struct e1000_tx_desc
{
    uint64_t buffer_addr;       /* Address of the descriptor's data buffer */
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
}__attribute__((packed));
/**
 * @brief Used by the NIC to store important information about 
 * the current state of the transmit buffers.
 */

/* receive descriptor */
struct e1000_rx_desc
{
	uint64_t buffer_addr;

	uint16_t length;             /* Data buffer length */
	uint16_t chksum;             /* Check Sum */

	uint8_t  status;
	uint8_t  err;
	uint16_t special;
}__attribute__((packed));
/**
 * @brief Used by the NIC to store important information
 * about the current state of the recieve buffers.
 */


void e1000_attach(struct pci_device* dev);
#endif