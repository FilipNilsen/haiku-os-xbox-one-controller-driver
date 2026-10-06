struct_name = "message"
struct = """
	uint8_t message_type;
	uint8_t flags;
	uint8_t sequence_id;
	uint8_t payload_length;
"""

for line in struct.split('\n'):
	if ';' in line:
		name = line.split()[1].strip(";")
		print(f'TRACE((DRIVER_NAME": {name}: 0x%x\\n", {struct_name}->{name}));')