

void live_send(void) {
	int8 buff[15];
	int16 lCRC;
	int8 i;
	int16 l;
	/* local copy of current */
	int16 pulse_period,pulse_min_period,pulse_count;

	/* shut off main timer so we don't change while we copy */
	disable_interrupts(INT_TIMER2);
	pulse_period = current.pulse_period;
	pulse_min_period = current.pulse_min_period;
	pulse_count = current.pulse_count_live;

	/* reset our current record */
	current.pulse_count_live=0;
	enable_interrupts(INT_TIMER2);

/* 
'#'             0  STX
UNIT ID PREFIX  1  First character (A-Z) for serial number
UNIT ID MSB     2  high byte of sending station ID
UNIT ID LSB     3  low byte of sending station ID
PACKET LENGTH   4  number of byte for packet including STX through CRC (13)
PACKET TYPE     5  type of packet we are sending (0x07)
WS MSB          6  high byte of wind speed time
WS LSB          7  low byte of wind speed time
WG MSB          8  high byte of wind gust time
WG LSB          9  low byte of wind gust time
BATT / WD       10 battery state of charge and wind direction sector
WC MSB          11 high byte of wind pulse count
WC LSB          12 low byte of wind pulse count
CRC MSB         13 high byte of CRC on everything after STX and before CRC
CRC LSB         14 low byte of CRC 
*/

//	output_high(WV1_FILTERED);


	buff[0]='#';
	buff[1]=current.serial_prefix;
	buff[2]=current.serial_msb;
	buff[3]=current.serial_lsb;
	buff[4]=15; /* packet length */
	buff[5]=0x07; /* packet type */

	if ( ANEMOMETER_TYPE_THEIS == current.anemometer_type ) {
		/* scale Theis anemometer frequency to #40HC recipricol frequency */
		l=anemometer_theis_to_40HC(pulse_period);
		buff[6]=make8(l,1); /* wind speed */
		buff[7]=make8(l,0);

		l=anemometer_theis_to_40HC(pulse_min_period);
		buff[8]=make8(l,1); /* wind gust */
		buff[9]=make8(l,0);

	} else {
		buff[6]=make8(pulse_period,1); /* wind speed */
		buff[7]=make8(pulse_period,0);
		buff[8]=make8(pulse_min_period,1); /* wind gust */
		buff[9]=make8(pulse_min_period,0); 
	}
	buff[10]=(current.battery_charge<<4) + (current.wind_direction_sector & 0x0F); /* battery % full, wind direction sector */
	buff[11]=make8(pulse_count,1); /* wind pulse count */
	buff[12]=make8(pulse_count,0); 

	lCRC=crc_chk(buff+1,12);
	buff[13]=make8(lCRC,1);
	buff[14]=make8(lCRC,0);


	for ( i=0 ; i<sizeof(buff) ; i++ ) {
		/* xbee modem */
		fputc(buff[i],stream_wireless);
	}	

	/* modem will automatically shut off when countdown done */



//	output_low(WV1_FILTERED);
}

void liveTask(void) {
	static short state=0;

	if ( 0 == state ) {
		/* wake up the modem */
		wirelessOn(1);
		wireless.age_response=0;
		state=1;
		return;
	}

	if ( wireless.age_response >= 14 ) {
		/* delay for >=14 milliseconds, then transmit */
		action.now_live=0;
		sample_adc();
		live_send();
		state=0;
	}	
}

/*		
rdLoggerCellStatus packet format
'#'                 0  STX
UNIT ID PREFIX      1  First character (A-Z) for serial number
UNIT ID MSB         2  sending station ID MSB
UNIT ID LSB         3  sending station ID LSB
PACKET LENGTH       4  number of byte for packet including STX through CRC (43)
PACKET TYPE         5  type of packet we are sending (0x08)

tPulseTime MSB      6  COUNTER pulse time
tPulseTime LSB      7  COUNTER pulse time
tPulseMinTime MSB   8  COUNTER pulse minimum time
tPulseMinTime LSB   9  COUNTER pulse minimum time
pulseCount MSB      10 COUNTER pulse count
pulseCount LSB      11 COUNTER pulse count

year                12 RTC year
month               13 RTC month
day                 14 RTC day
hour                15 RTC hour
minute              16 RTC minute
second              17 RTC second

dataflashReadStatus 18 DATAFLASH status (172 is normal)
dataflashPage MSB   19 DATAFLASH last page being used
dataflashPage LSB   20 DATAFLASH last page being used

adcInputVoltage MSB 21 ADC input voltage (10 bits spanning 0 to 40 volts)
adcInputVoltage LSB 22 ADC input voltage (10 bits spanning 0 to 40 volts)
sdStatus            23 SD status

latitude AEXP       24 GPS latitude 8-bit biased exponent
latitude AARGB0     25 GPS latitude MSB of mantissa
latitude AARGB1     26 GPS latitude mantissa middle byte
latitude AARGB2     27 GPS latitude LSB of mantissa 
longitude AEXP      28 GPS longitude 8-bit biased exponent
longitude AARGB0    29 GPS longitude MSB of mantissa
longitude AARGB1    30 GPS longitude mantissa middle byte
longitude AARGB2    31 GPS longitude LSB of mantissa 

windDirectionSector 32 Wind direction sector
gprsState           33 GPRS connection state
gprsUptime MSB      34 GPRS uptime minutes 
gprsUptime LSB      35
compileYear         36 Firmware compiled year
compileMonth        37 Firmware compiled month
compileDay          38 Firmware compiled day

uptime MSB          39 unit uptime minutes
uptime LSB          40

CRC MSB             41 high byte of CRC on everything after STX and before CRC
CRC LSB             42 low byte of CRC

*/

void live_send_status(void) {
	int8 buff[43];
	int16 lCRC;
	int8 i;
	int8 *p;

	buff[0]='#';
	buff[1]=current.serial_prefix;
	buff[2]=current.serial_msb;
	buff[3]=current.serial_lsb;
	buff[4]=sizeof(buff); /* packet length */
	buff[5]=0x08; /* packet type */

	buff[6]=make8(current.pulse_period,1); /* wind speed */
	buff[7]=make8(current.pulse_period,0);
	buff[8]=make8(current.pulse_min_period,1); /* wind gust */
	buff[9]=make8(current.pulse_min_period,0); 
	buff[10]=make8(current.pulse_count_live,1); /* wind pulse count */
	buff[11]=make8(current.pulse_count_live,0); 

	buff[12]=timers.year;
	buff[13]=timers.month;
	buff[14]=timers.day;
	buff[15]=timers.hour;
	buff[16]=timers.minute;
	buff[17]=timers.second;

	buff[18]=dataflash_read_status();
	buff[19]=read_ext_fram(FRAM_ADDR_DATAFLASH_PAGE+0);
	buff[20]=read_ext_fram(FRAM_ADDR_DATAFLASH_PAGE+1);

	buff[21]=make8(current.input_voltage_adc,1);
	buff[22]=make8(current.input_voltage_adc,0);
	buff[23]=input(MMC_STATUS_TO_HOST);

	p=(int8 *)&gps.latitude;
	buff[24]=p[0];
	buff[25]=p[1];
	buff[26]=p[2];
	buff[27]=p[3];

	p=(int8 *)&gps.longitude;
	buff[28]=p[0];
	buff[29]=p[1];
	buff[30]=p[2];
	buff[31]=p[3];

	buff[32]=current.wind_direction_sector;
	buff[33]=0; // gprs.state;
	buff[34]=0; // make8(gprs.uptime,1);
	buff[35]=0; // make8(gprs.uptime,0);
	buff[36]=current.compile_year;
	/* low four bits is the compile month, high four bits is the high nibble of the modem on time */
	buff[37]=(current.compile_month&0x0f) + (timers.modem_seconds&0xf0);
	buff[38]=current.compile_day;
	buff[39]=make8(current.uptime,1);
	buff[40]=make8(current.uptime,0);


	lCRC=crc_chk(buff+1,40);
	buff[41]=make8(lCRC,1);
	buff[42]=make8(lCRC,0);

	for ( i=0 ; i<sizeof(buff) ; i++ ) {
		/* xbee modem */
		fputc(buff[i],stream_wireless);
	}	

	/* modem will automatically shut off when countdown done */
}

void liveStatusTask(void) {
	static short state=0;

	if ( 0 == state ) {
		/* wake up the modem */
		wireless.age_response=0;
		wirelessOn(1);
		state=1;
		return;
	}

	if ( wireless.age_response >= 14 ) {
		/* delay for >=14 milliseconds, then transmit */
		action.now_live_status=0;
		live_send_status();
		state=0;
	}	
}