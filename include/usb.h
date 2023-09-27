#ifndef __USB_H
#define __USB_H

#include <stdint.h>

/* USB Request Types */
#define USB_REQ_GET_STATUS        0x00
#define USB_REQ_CLEAR_FEATURE     0x01
#define USB_REQ_SET_FEATURE       0x03
#define USB_REQ_SET_ADDRESS       0x05
#define USB_REQ_GET_DESCRIPTOR    0x06
#define USB_REQ_SET_DESCRIPTOR    0x07
#define USB_REQ_GET_CONFIGURATION 0x08
#define USB_REQ_SET_CONFIGURATION 0x09

/* USB Descriptor Types */
#define USB_DESC_TYPE_DEVICE      0x01
#define USB_DESC_TYPE_CONFIG      0x02
#define USB_DESC_TYPE_STRING      0x03
#define USB_DESC_TYPE_INTERFACE   0x04
#define USB_DESC_TYPE_ENDPOINT    0x05

/* USB Endpoint Types */
#define USB_ENDPOINT_TYPE_CONTROL 0x00
#define USB_ENDPOINT_TYPE_ISOCHRONOUS 0x01
#define USB_ENDPOINT_TYPE_BULK    0x02
#define USB_ENDPOINT_TYPE_INTERRUPT 0x03

/* USB Device States */
typedef enum usb_device_states {
    USB_STATE_ATTACHED,
    USB_STATE_POWERED,
    USB_STATE_DEFAULT,
    USB_STATE_ADDRESS,
    USB_STATE_CONFIGURED
} usb_state_t;

/* USB Device Descriptor */
struct usb_device_descriptor{
    uint8_t length;
    uint8_t descriptor_type;
    uint16_t usb_version; // renamed from bcdUSB for clarity
    uint8_t device_class;
    uint8_t device_sub_class;
    uint8_t device_protocol;
    uint8_t max_packet_size0;
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t device_version; // renamed from bcdDevice for clarity
    uint8_t manufacturer_index;
    uint8_t product_index;
    uint8_t serial_number_index;
    uint8_t num_configurations;
};

/* USB Configuration Descriptor */
struct usb_config_descriptor {
    uint8_t length;
    uint8_t descriptor_type;
    uint16_t total_length;
    uint8_t num_interfaces;
    uint8_t configuration_value;
    uint8_t configuration_index;
    uint8_t attributes;
    uint8_t max_power;
};

/* USB Device */
struct usb_device {
    usb_state_t state;
    struct usb_device_descriptor device_desc;
    struct usb_config_descriptor config_desc;
    uint8_t address;
};


/* USB Operations */
struct usb_controller_ops {
    int (*initialize)(struct usb_device* dev);
    int (*enumerate)(struct usb_device* dev);
    int (*control_transfer)(struct usb_device* dev, ...);
    int (*bulk_transfer)(struct usb_device* dev, ...);
};

struct usb_controller {
    struct usb_controller_ops* ops;
    struct usb_device* attached_devices;
} ;

int usb_init(struct usb_controller* controller);
int usb_attach_device(struct usb_device* dev);
int usb_detach_device(struct usb_device* dev);

#endif
