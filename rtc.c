/** rtc.c	Access the DS1307 realtime clock / NVRAM.
 * @author James Jefferson Jarvis <jj@aprsworld.com>
 */

#define RTC_READ_CMD 0xd1
#define RTC_WRITE_CMD 0xd0

void write_rtc(int8 address, int8 data) {
    short ackbit;

    do {
       i2c_start();
       ackbit = i2c_write(0xd0);
    } while(ackbit);

    i2c_write(address);
    i2c_write(data);
    i2c_stop();

    return;
}



int8 read_rtc(int8 address) {
	int value;

	// writer the read address for the seconds register (0x00)
	do {
		i2c_start();
	} while (1 == i2c_write(0xd0)); // poll for ack
    
	i2c_write(address);                    // seconds address
     
      // read the seconds from the RTC and output them to RS232

	do {
		i2c_start();
	} while (1 == i2c_write(0xd1)); // poll for ack     

    value=i2c_read(0);     // read the hours
	i2c_stop();                         // end of transaction
    

	return value;
}


typedef union {
	int32 l;
	float f;
} union_int32_float;


float rtc_read_float(int8 addr) {
	int8 a,b,c,d;
	union_int32_float u;

	a=read_rtc(addr+0); // MSB
	b=read_rtc(addr+1);
	c=read_rtc(addr+2);
	d=read_rtc(addr+3); // LSB


	u.l = make32(a,b,c,d);

	return u.f;
}

void rtc_write_float(int8 addr, union_int32_float u) {
	write_rtc(addr+0,make8(u.l,3)); // MSB at address+0
	write_rtc(addr+1,make8(u.l,2));
	write_rtc(addr+2,make8(u.l,1));
	write_rtc(addr+3,make8(u.l,0)); // LSB at address+3
}

int32 rtc_read_int32(int8 addr) {
	int8 a,b,c,d;

	a=read_rtc(addr+0); // MSB
	b=read_rtc(addr+1);
	c=read_rtc(addr+2);
	d=read_rtc(addr+3); // LSB
	return make32(a,b,c,d);
}

void rtc_write_int32(int8 addr, int32 val) {
	write_rtc(addr+0,make8(val,3)); // MSB at address+0
	write_rtc(addr+1,make8(val,2));
	write_rtc(addr+2,make8(val,1));
	write_rtc(addr+3,make8(val,0)); // LSB at address+3
}

int16 rtc_read_int16(int8 addr) {
	int8 a,b;

	a=read_rtc(addr+0);
	b=read_rtc(addr+1);
	return make16(a,b);
}

void rtc_write_int16(int8 addr, int16 val) {
	write_rtc(addr+0,make8(val,1));
	write_rtc(addr+1,make8(val,0));
}


 
void init_rtc(void) {
	write_rtc(0x07, 0x10); /* enable SQWE with one second pulse */	
}

/* Address map:
00 Seconds
01 Minutes
02 Hours
03 Day
04 Date
05 Month
06 Year

07 Control
08 - 3F RAM 56x8
*/	

char bin2bcd(char binary_value) {
	char temp;
	char retval;

	temp = binary_value;
	retval = 0;

#IGNORE_WARNINGS 203 
	while(1) {
#IGNORE_WARNINGS NONE
		if(temp >= 10) {
			// Get the tens digit by doing multiple subtraction of 10 from the binary value
			temp -= 10;
			retval += 0x10;
		} else {
			// Get the ones digit by adding the remainder.
			retval += temp;
			break;
		}
	}
	return(retval);
}

int8 bcd2bin(int8 bcd_value) {
	int8 tens;
	int8 ones;

	tens=bcd_value>>4;
	ones=bcd_value&0x0f;

	return (10*tens + ones);
}

#inline
int get_second_rtc(void) {
	return read_rtc(0x00) & 0b01111111;
}

#inline
int get_minute_rtc(void) {
	return read_rtc(0x01);
}

#inline
int get_hour_rtc(void) {
	return read_rtc(0x02) & 0b00111111 ;
}

#inline
int get_date_rtc(void) {
	return read_rtc(0x04);
}

#inline 
int get_month_rtc(void) {
	return read_rtc(0x05);
}

#inline 
int get_year_rtc(void) {
	return read_rtc(0x06);
}


/* read real time clock and update timers structure */
void update_time_rtc(void) {
	timers.hour=bcd2bin(read_rtc(0x02) & 0b00111111);
	timers.minute=bcd2bin(read_rtc(0x01));
	timers.second=bcd2bin(read_rtc(0x00) & 0b01111111);
	timers.day=bcd2bin(read_rtc(0x04));
	timers.month=bcd2bin(read_rtc(0x05));
	timers.year=bcd2bin(read_rtc(0x06));
}


void reset_rtc(void) {
//		fputs("# invalid date, resetting clock",rs232);
		write_rtc(0x00,0x00);	/* enable oscillator and set seconds to zero */
		write_rtc(0x06,0x0a);	/* 2010 */
		write_rtc(0x05,0x01);	/* January */
		write_rtc(0x04,0x01);	/* 1st */
		write_rtc(0x02,0x00);   /* 00 */
		write_rtc(0x01,0x00);   /* 00 */
}


