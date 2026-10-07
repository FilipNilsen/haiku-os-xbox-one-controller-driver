struct_name = "message"
struct = """
	uint8_t button_lsb;
	uint8_t button_msb;
	uint16_t left_trigger;
	uint16_t right_trigger;
	uint16_t left_thumbstick_x;
	uint16_t left_thumbstick_y;
	uint16_t right_thumbstick_x;
	uint16_t right_thumbstick_y;
"""

for line in struct.split('\n'):
	if ';' in line:
		name = line.split()[1].strip(";")
		print(f'TRACE((DRIVER_NAME": {name}: 0x%x\\n", {struct_name}->{name}));')