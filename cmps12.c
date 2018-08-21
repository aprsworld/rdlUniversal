/* Command register (write) / Software version (read) */
#define CMPS12_REG_COMMAND_VERSION     0x00

/* Compass Bearing 8 bit, i.e. 0-255 for a full circle */
#define CMPS12_REG_BEARING             0x01

/* Compass Bearing 16 bit, i.e. 0-3599, representing 0-359.9 degrees. register 2 being the 
high byte. This is calculated by the processor from quaternion outputs of the BNO055 */
#define CMPS12_REG_BEARING_MSB         0x02
#define CMPS12_REG_BEARING_LSB         0x03

/* Pitch angle - signed byte giving angle in degrees from the horizontal plane (+/- 90°) */
#define CMPS12_REG_PITCH               0x04

/* Roll angle - signed byte giving angle in degrees from the horizontal plane (+/- 90°) */
#define CMPS12_REG_ROLL                0x05

/* Magnetometer X axis raw output, 16 bit signed integer (register 0x06 high byte) */
#define CMPS12_REG_MAGNETOMETER_X_MSB  0x06
#define CMPS12_REG_MAGNETOMETER_X_LSB  0x07

/* Magnetometer Y axis raw output, 16 bit signed integer (register 0x08 high byte) */
#define CMPS12_REG_MAGNETOMETER_Y_MSB  0x08
#define CMPS12_REG_MAGNETOMETER_Y_LSB  0x09

/* Magnetometer Z axis raw output, 16 bit signed integer (register 0x0A high byte) */
#define CMPS12_REG_MAGNETOMETER_Z_MSB  0x0A
#define CMPS12_REG_MAGNETOMETER_Z_LSB  0x0B

/* Accelerometer X axis raw output, 16 bit signed integer (register 0x0C high byte) */
#define CMPS12_REG_ACCELEROMETER_X_MSB 0x0C
#define CMPS12_REG_ACCELEROMETER_X_LSB 0x0D

/* Accelerometer Y axis raw output, 16 bit signed integer (register 0x0E high byte) */
#define CMPS12_REG_ACCELEROMETER_Y_MSB 0x0E
#define CMPS12_REG_ACCELEROMETER_Y_LSB 0x0F

/* Accelerometer Z axis raw output, 16 bit signed integer (register 0x10 high byte) */
#define CMPS12_REG_ACCELEROMETER_Z_MSB 0x10
#define CMPS12_REG_ACCELEROMETER_Z_LSB 0x11

/* Gyro X axis raw output, 16 bit signed integer (register 0x12 high byte) */
#define CMPS12_REG_GYRO_X_MSB          0x12
#define CMPS12_REG_GYRO_X_LSB          0x13

/* Gyro Y axis raw output, 16 bit signed integer (register 0x14 high byte) */
#define CMPS12_REG_GYRO_Y_MSB          0x14
#define CMPS12_REG_GYRO_Y_LSB          0x15

/* Gyro Z axis raw output, 16 bit signed integer (register 0x16 high byte) */
#define CMPS12_REG_GYRO_Z_MSB          0x16
#define CMPS12_REG_GYRO_Z_LSB          0x17

/* Temperature of the BNO055 in degrees centigrade */
#define CMPS12_REG_TEMPERATURE         0x19

/* Compass Bearing 16 bit This is the angle Bosch generate in the BNO055 (0-5759), 
divide by 16 for degrees */
#define CMPS12_REG_BNO055_COMPASS_MSB  0x1A
#define CMPS12_REG_BNO055_COMPASS_LSB  0x1B

/* Pitch angle 16 bit - signed byte giving angle in degrees from the horizontal plane (+/- 
180°) */
#define CMPS12_REG_PITCH_ANGLE_MSB     0x1C
#define CMPS12_REG_PITCH_ANGLE_LSB     0x1D

/* Calibration state, bits 0 and 1 reflect the calibration status (0 un-calibrated, 3 fully 
calibrated) */
#define CMPS12_REG_CALIBRATION_STATE   0x1E


int8 cmps12_get_int8(int8 addr) {
	int8 value;

	do {
		i2c_start();
	} while (1 == i2c_write(I2C_ADDR_CMPS12)); 
    
	/* address to read */
	i2c_write(addr);  
     
	do {
		i2c_start();
	} while (1 == i2c_write(I2C_ADDR_CMPS12+1)); 

    value=i2c_read(0);
	i2c_stop();
    
	return value;
}

int16 cmps12_get_int16(int8 addr) {
	int16 value;

	value=make16(cmps12_get_int8(addr),cmps12_get_int8(addr+1));


	return value;
}

#inline
int8 cmps12_get_version() {
	return cmps12_get_int8(CMPS12_REG_COMMAND_VERSION);
}
