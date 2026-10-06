struct_name = "interface->endpoint[i].descr"
struct = """
	uint8	length;
	uint8	descriptor_type;
	uint8	endpoint_address;
	uint8	attributes;
	uint16	max_packet_size;
	uint8	interval;
"""

for line in struct.split('\n'):
	if ';' in line:
		name = line.split()[1].strip(";")
		print(f'TRACE((DRIVER_NAME": {name}: 0x%x\\n", {struct_name}->{name}));')