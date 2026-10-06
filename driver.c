/* ++++++++++
	driver.c
	A skeletal device driver

	Copyright 2024, My Name 
	All rights reserved. Distributed under the terms of the MIT license.
+++++ */

#include <Drivers.h>
#include <Errors.h>
#include <KernelExport.h>
#include <USB3.h>
#include <stdio.h>
#include <stdlib.h>

#define DRIVER_NAME "GIPUSB"
#define TRACE(x)	dprintf x

                                  //76543210
#define GIP_FLAG_FRAGMENTED       0b10000000
#define GIP_FLAG_INITIAL_FRAGMENT 0b01000000
#define GIP_FLAG_SYSTEM           0b00100000
#define GIP_FLAG_ACK_ME           0b00010000
                                        //76543210
#define GIP_DATA_CLASS_COMMAND          0b00000000
#define GIP_DATA_CLASS_LOW_LATENCY      0b00100000
#define GIP_DATA_CLASS_STANDARD_LATENCY 0b01000000
#define GIP_DATA_CLASS_AUDIO            0b01100000

// The rest of the MessageType is the message number
								   //76543210
#define GIP_MESSAGE_NUMBER_BITMASK 0b00011111

usb_module_info *gUsb;
const char *kDriverName = DRIVER_NAME;

static usb_support_descriptor sSupportedDevices[1] = {
    { 0, 0, 0, 0x45e, 0x2ea },
};

bool shutting_down = false;

struct __attribute__((__packed__)) gip_message_header {
	uint8_t message_type;
	uint8_t flags;
	uint8_t sequence_id;
	uint8_t payload_length;
};

struct __attribute__((__packed__)) gip_hello_device_payload {
	uint64_t device_id;
	uint16_t vendor_id;
	uint16_t product_id;
	uint16_t firmware_version_major;
	uint16_t firmware_version_minor;
	uint16_t firmware_version_build;
	uint16_t firmvare_revision;
	uint8_t hardware_version_major;
	uint8_t hardvare_version_minor;
	uint8_t rf_protocol_version_major;
	uint8_t rf_protocol_version_minor;
	uint8_t security_protocol_version_major;
	uint8_t security_protocol_version_minor;
	uint8_t gip_version_minor;
	uint8_t gip_version_major;
};

// This causes a crash for the OS when used.
void
dump_usb_data(void *cookie, status_t status, void *data, size_t actualLength){
	if (status != B_OK) {
		TRACE((DRIVER_NAME": Status is %d", status));
		goto setup_next;
	}
	
	char *message = malloc((actualLength * 3) + 1);
	memset(message, 0, (actualLength * 3) + 1);
	for (size_t i = 0; i < actualLength; i++){
		snprintf(message + (i * 3), (actualLength * 3) + 1, "%02x ", ((uint8_t*)data)[i]);
	}
	TRACE((DRIVER_NAME": %s\n", message));
	free(message);
	
	message = malloc(actualLength + 1);
	memcpy(message, data, actualLength);
	message[actualLength + 1] = 0;
	TRACE((DRIVER_NAME": %s\n", message));
	free(message);
	
	setup_next:
	if (!shutting_down)
		gUsb->queue_interrupt(*(usb_pipe *)(cookie), data, 64, dump_usb_data, cookie);
	return;
}

void
get_gip_hello(void *cookie, status_t status, void *data, size_t actualLength){
	if (status != B_OK)
		return;
	struct gip_message_header *message = (struct gip_message_header *)data;
	TRACE((DRIVER_NAME": message_type: 0x%x\n", message->message_type));
	TRACE((DRIVER_NAME": flags: 0x%x\n", message->flags));
	TRACE((DRIVER_NAME": sequence_id: 0x%x\n", message->sequence_id));
	TRACE((DRIVER_NAME": payload_length: 0x%x\n\n", message->payload_length));
}

static status_t gipusb_device_added(const usb_device dev, void **cookie) {
	TRACE((DRIVER_NAME": device 0x%x connected\n", dev));
	
	const usb_device_descriptor* descriptor = gUsb->get_device_descriptor(dev);
	TRACE((DRIVER_NAME": length: 0x%x\n", descriptor->length));
	TRACE((DRIVER_NAME": descriptor_type: 0x%x\n", descriptor->descriptor_type));
	TRACE((DRIVER_NAME": usb_version: 0x%x\n", descriptor->usb_version));
	TRACE((DRIVER_NAME": device_class: 0x%x\n", descriptor->device_class));
	TRACE((DRIVER_NAME": device_subclass: 0x%x\n", descriptor->device_subclass));
	TRACE((DRIVER_NAME": device_protocol: 0x%x\n", descriptor->device_protocol));
	TRACE((DRIVER_NAME": max_packet_size_0: 0x%x\n", descriptor->max_packet_size_0));
	TRACE((DRIVER_NAME": vendor_id: 0x%x\n", descriptor->vendor_id));
	TRACE((DRIVER_NAME": product_id: 0x%x\n", descriptor->product_id));
	TRACE((DRIVER_NAME": device_version: 0x%x\n", descriptor->device_version));
	TRACE((DRIVER_NAME": manufacturer: 0x%x\n", descriptor->manufacturer));
	TRACE((DRIVER_NAME": product: 0x%x\n", descriptor->product));
	// TRACE((DRIVER_NAME": serial_number: 0x%x\n", descriptor->serial_number));
	TRACE((DRIVER_NAME": num_configurations: 0x%x\n", descriptor->num_configurations));
	
	const usb_configuration_info *config = gUsb->get_configuration(dev);
	TRACE((DRIVER_NAME": Number of interfaces: %d\n", config->interface_count));
	
	const usb_interface_list *interfaces = config->interface;
	TRACE((DRIVER_NAME": We have 1 primary interface at 0x%x "
	"and %d alternative ones starting at 0x%x\n",
	interfaces->active, interfaces->alt_count, interfaces->alt));
	
	const usb_interface_info *interface = interfaces->active;
	TRACE((DRIVER_NAME": The primary interface has %d endpoints\n\n", interface->endpoint_count));
	
	for (size_t i = 0; i < interface->endpoint_count; i++){
		TRACE((DRIVER_NAME": Endpoint %d has id 0x%x\n", i, interface->endpoint[i]));
		TRACE((DRIVER_NAME": length: 0x%x\n", interface->endpoint[i].descr->length));
		TRACE((DRIVER_NAME": descriptor_type: 0x%x\n", interface->endpoint[i].descr->descriptor_type));
		TRACE((DRIVER_NAME": endpoint_address: 0x%x\n", interface->endpoint[i].descr->endpoint_address));
		TRACE((DRIVER_NAME": attributes: 0x%x\n", interface->endpoint[i].descr->attributes));
		TRACE((DRIVER_NAME": max_packet_size: 0x%x\n", interface->endpoint[i].descr->max_packet_size));
		TRACE((DRIVER_NAME": interval: 0x%x\n\n", interface->endpoint[i].descr->interval));

		if (true){
			uint8_t *data = malloc(64);
			size_t *cookie = malloc(sizeof(interface->endpoint[i].handle));
			*cookie = interface->endpoint[i].handle;
			gUsb->queue_interrupt(interface->endpoint[i].handle, data, 64, get_gip_hello, cookie);
		}
	}
	return B_OK;
}
static status_t gipusb_device_removed(void *cookie) {
	shutting_down = true;
	TRACE((DRIVER_NAME": device disconnected\n"));
	return B_OK;
}
 
static usb_notify_hooks sNotifyHooks = {
    gipusb_device_added,
    gipusb_device_removed
};
 

/* ----------
	init_hardware - called once the first time the driver is loaded
----- */
status_t
init_hardware()
{
	return B_OK;
}


/* ----------
	init_driver - optional function - called every time the driver
	is loaded.
----- */
status_t
init_driver()
{
	TRACE((DRIVER_NAME": driver loaded\n"));
	if (get_module(B_USB_MODULE_NAME, (module_info **)&gUsb) != B_OK)
		return B_ERROR;
		
	gUsb->register_driver(kDriverName, sSupportedDevices, 1, NULL);
	gUsb->install_notify(kDriverName, &sNotifyHooks);
	return B_OK;
}


/* ----------
	uninit_driver - optional function - called every time the driver
	is unloaded
----- */
void
uninit_driver()
{
    gUsb->uninstall_notify(kDriverName);
    put_module(B_USB_MODULE_NAME);
}


/* ----------
	my_device_open - handle open() calls
----- */
static status_t
my_device_open(const char* name, uint32 flags, void** cookie)
{
	return B_OK;
}


/* ----------
	my_device_read - handle read() calls
----- */
static status_t
my_device_read(void* cookie, off_t position, void* buf, size_t* num_bytes)
{
	*num_bytes = 0; /* tell caller nothing was read */
	return B_IO_ERROR;
}


/* ----------
	my_device_write - handle write() calls
----- */
static status_t
my_device_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	*num_bytes = 0; /* tell caller nothing was written */
	return B_IO_ERROR;
}


/* ----------
	my_device_control - handle ioctl calls
----- */
static status_t
my_device_control(void* cookie, uint32 op, void* arg, size_t len)
{
	return B_BAD_VALUE;
}


/* ----------
	my_device_close - handle close() calls
----- */
static status_t
my_device_close(void* cookie)
{
	return B_OK;
}


/* -----
	my_device_free - called after the last device is closed, and after
	all i/o is complete.
----- */
static status_t
my_device_free(void* cookie)
{
	return B_OK;
}


/* -----
	null-terminated array of device names supported by this driver
----- */
static const char* my_device_name[] = {
	"bogus/my_device",
	NULL
};


/* -----
	function pointers for the device hooks entry points
----- */
device_hooks my_device_hooks = {
	my_device_open, /* -> open entry point */
	my_device_close, /* -> close entry point */
	my_device_free, /* -> free cookie */
	my_device_control, /* -> control entry point */
	my_device_read, /* -> read entry point */
	my_device_write /* -> write entry point */
};


/* ----------
	publish_devices - return a null-terminated array of devices
	supported by this driver.
----- */
const char**
publish_devices()
{
	return my_device_name;
}


/* ----------
	find_device - return ptr to device hooks structure for a
	given device name
----- */
device_hooks*
find_device(const char* name)
{
	return &my_device_hooks;
}