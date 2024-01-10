#include <usb.h>
#include <pci.h>
#include <kutils.h>
#include <libc.h>
#include <errors.h>
#include <serial.h>

#define USB_CTRL_REG_INIT    0x00
#define USB_CTRL_REG_STATUS  0x04
#define USB_CTRL_REG_ADDRESS 0x08

#define USB_CTRL_INIT_VALUE  0x01
#define USB_CTRL_RESET_VALUE 0x02

static struct usb_controller_ops usb_ops;

volatile uint32_t* USB_CONTROLLER_BASE = NULL;

/* Initialize the USB controller */
int usb_controller_init(struct usb_controller* controller)
{
    ERR_ON_NULL(controller);

    /* Set up the necessary data structures */
    controller->ops = &usb_ops;

    /* Configure the controller's registers */
    USB_CONTROLLER_BASE[USB_CTRL_REG_INIT] = USB_CTRL_INIT_VALUE;

    return 0; /* Success */
}


static int usb_read_device_descriptor(struct usb_device* device)
{
    /* Implement the logic to read the device descriptor here */
    return 0; /* Placeholder return */
}

static int usb_read_config_descriptor(struct usb_device* device)
{
    /* Implement the logic to read the configuration descriptor here */
    return 0; /* Placeholder return */
}


/* Enumerate the attached USB device */
static int usb_device_enumerate(struct usb_device* device) {
    ERR_ON_NULL(device);

    /* Reset the USB port */
    USB_CONTROLLER_BASE[USB_CTRL_REG_STATUS] = USB_CTRL_RESET_VALUE;

    /* wait */
    for (volatile int i = 0; i < 10000; i++);

    /* Read the device descriptor */
    if (usb_read_device_descriptor(device) != 0) {
        return -2; /* Error reading device descriptor */
    }

    /* Assign a unique address to the device */
    static uint16_t next_address = 1; /* Start from address 1 as address 0 is default for new devices */
    device->address = next_address++;
    USB_CONTROLLER_BASE[USB_CTRL_REG_ADDRESS] = device->address;

    /* Read the configuration descriptor and other relevant descriptors */
    if (usb_read_config_descriptor(device) != 0) {
        return -3; /* Error reading configuration descriptor */
    }

    return 0; /* Success */
}

void usb_attach(struct pci_device* dev)
{
    USB_CONTROLLER_BASE = (uint32_t*)dev->base;
}