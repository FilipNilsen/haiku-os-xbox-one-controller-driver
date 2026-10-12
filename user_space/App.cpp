/*
 * Copyright 2024, My Name 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <USBKit.h>
// #include <cstdio>
#include <iostream>
// #define TRACE(x)	dprintf x
#include <ByteOrder.h>
#include <iomanip>

struct __attribute__((__packed__)) gip_message_header {
	uint8_t message_type;
	uint8_t flags;
	uint8_t sequence_id;
	uint8_t payload_length;
};

struct __attribute__((__packed__)) gip_hello_device {
	struct gip_message_header header;
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

struct __attribute__((__packed__)) gip_input_report {
	struct gip_message_header header;
	uint8_t button_lsb;
	uint8_t button_msb;
	uint16_t left_trigger;
	uint16_t right_trigger;
	uint16_t left_thumbstick_x;
	uint16_t left_thumbstick_y;
	uint16_t right_thumbstick_x;
	uint16_t right_thumbstick_y;
};

class XboxController {
	private:
	BUSBDevice* _usb_device;
	const BUSBEndpoint* _input;
	const BUSBEndpoint* _output;
	uint8_t _input_buffer[64] = {0};
	uint8_t _gip_global_sequence = 1;
	uint8_t _gip_sequence_for_message[255] = {1};
	
	void Start() {
		uint8_t message[] = {0x5,0x20,_gip_global_sequence++,0x01,0x0};
		_output->InterruptTransfer(message, 5);
	}
	
	void HandleHello(struct gip_hello_device* message){
		uint64_t id = message->device_id;
		swap_data(B_UINT64_TYPE, &id, 8, B_SWAP_LENDIAN_TO_HOST);
		std::cout<<std::hex<<std::setw(16)<<std::setfill('0')<<id<<std::endl;
		Start();
	}
	
	void HandleInput(struct gip_input_report* message) {
		uint16_t* a = &(message->left_trigger);
		for (size_t i = 0; i < 6; ++i){
			*a = (*a<<8)|(*a>>8);
		}
	}
	
	void PoolController() {
		ssize_t ret =_input->InterruptTransfer(_input_buffer, 64);
		struct gip_message_header *message = (struct gip_message_header*)_input_buffer;
		std::cout<<ret<<" Message type "<<(int)(message->message_type)<<std::endl;
		if (message->message_type == 0x02) {
			HandleHello((struct gip_hello_device*)message);
		}
		else if (message->message_type == 0x20) {
			HandleInput((struct gip_input_report*)message);
		}
	}
	
	public:
	void InputLoop() {
		while(1) {
			PoolController();
		}
	}
	
	XboxController(BUSBDevice* usb_device) {
		std::cout<<"XBOX controller added\n";
		this->_usb_device = usb_device;
		_input = _usb_device->ActiveConfiguration()->InterfaceAt(0)->EndpointAt(1);
		_output = _usb_device->ActiveConfiguration()->InterfaceAt(0)->EndpointAt(0);
		InputLoop();
	}
};



class GIPRoster : public BUSBRoster {
	public:
	status_t DeviceAdded(BUSBDevice* device) {
		std::cout<<"Device added\n";
		if (device->Class() == 0xff) {
			if (device->Subclass() == 0x47) {
				if (device->Protocol() == 0xd0) {
					if (device->VendorID() == 0x45e) {
						if (device->ProductID() == 0x2ea) {
							new XboxController(device);
						}
					}
				}
			}
		}
		return B_OK;
	}
	void DeviceRemoved(BUSBDevice*) {
		std::cout<<"Device removed\n";
	}
};

int main() {
	auto roster = new GIPRoster();
	roster->Start();
	std::cin.get();
	roster->Stop();
	return 0;
}