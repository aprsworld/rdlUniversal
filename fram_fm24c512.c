#define FRAM_SIZE     65535

void init_ext_fram() {
	output_float(I2C_SCL);
	output_float(I2C_SDA);
}


int8 read_ext_fram(int16 address) {
	int8 data;
	int8 slaveAddress = 0b10100000;

	/* bit 15 of address goes in slaveAddress */
	if ( address >= 0x8000 ) {
		slaveAddress = slaveAddress | 0x02;
		address=address & 0x7fff;
	}

	i2c_start();
	i2c_write(slaveAddress);
	i2c_write(address>>8);
	i2c_write(address);
	i2c_start();
	i2c_write(slaveAddress | 0x1);
	data=i2c_read(0);
	i2c_stop();

	return data;
}

void write_ext_fram(int16 address, int8 data) {
	int8 slaveAddress = 0b10100000;

	/* bit 15 of address goes in slaveAddress */
	if ( address >= 0x8000 ) {
		slaveAddress = slaveAddress | 0x02;
		address=address & 0x7fff;
	}

	i2c_start();
	i2c_write(slaveAddress);
	i2c_write(address>>8);
	i2c_write(address);
	i2c_write(data);
	i2c_stop();
}


