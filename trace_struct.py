struct_name = "hello"
struct = """
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
"""

for line in struct.split('\n'):
	if ';' in line:
		name = line.split()[1].strip(";")
		print(f'TRACE((DRIVER_NAME": {name}: 0x%x\\n", {struct_name}->{name}));')