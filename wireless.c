void wirelessOn(int8 seconds) {
	if ( seconds > timers.modem_seconds ) 
		timers.modem_seconds=seconds;

	output_low(MODEM_SLEEP);
//	output_low(LCD_BACKLIGHT);
}

void wirelessOff() {
	output_high(MODEM_SLEEP);
//	output_high(LCD_BACKLIGHT);
}


void wirelessMessageInit() {
	memset(wireless.message,'\0',sizeof(wireless.message));
	wireless.message_waiting=0;
}

void wirelessInit() {
	disable_interrupts(INT_RDA);

	memset(wireless.buff,'\0',sizeof(wireless.buff));
	wireless.buff_length=0;
	wireless.age=65535;

	wirelessMessageInit();

	enable_interrupts(INT_RDA);
}



void wirelessMessageFromBuff() {
	disable_interrupts(INT_RDA);
	/* make a new message if the last one has been read */
	if ( wireless.message_waiting != 1 ) {
		memcpy(wireless.message,wireless.buff,wireless.buff_length);
		wireless.message[wireless.buff_length+1]='\0'; // always null terminated
		wireless.message_waiting=1;

		/* reset our buffer pointer to beginning */
		wireless.buff_length=0;
	}	
	enable_interrupts(INT_RDA);
}

void wirelessTick(void) {
	int16 lcrc;

	/* check for requests from the wireless channel */
	if ( wireless.message_waiting ) {
		wireless.message_waiting=0;

		/* check to see if this is a query addressed to us */
		if ( '#' == wireless.message[0] &&
             current.serial_prefix == wireless.message[1] &&
             current.serial_msb == wireless.message[2] &&
             current.serial_lsb == wireless.message[3] &&
             13 == wireless.message[4] &&
             22 == wireless.message[5] 
           ) {
			/* query to us */

		
			lcrc=crc_chk(wireless.message+1,10);

			if ( lcrc != make16(wireless.message[11],wireless.message[12]) ) {
//				fprintf(stream_wireless,"# invalid crc ... we calculated 0x%04lx and got 0x%04lx\r\n",lcrc,make16(wireless.message[11],wireless.message[12]));
				return;
			}


			/* okay, so let's turn on the modem for a while */
			wirelessOn(250);

			switch ( wireless.message[6] ) {
				case 0: 
					action.now_live_status=1;
					break;
				case 1:
					action.now_log_init=1;
					break;
				case 2:
					log.page_requested=make16(wireless.message[7],wireless.message[8]);
					log.n_pages=make16(wireless.message[9],wireless.message[10]);
					break;
			}
		}
	}
}


/* 
Query addressed to us:
'#'             0  STX
UNIT ID PREFIX  1  First character (A-Z) of OUR serial number
UNIT ID MSB     2  OUR serial number
UNIT ID LSB     3  
PACKET LENGTH   4  number of byte for packet including STX through CRC (13)
PACKET TYPE     5  type of packet we are sending (22)
COMMAND         6  function to perform
PARAM A MSB     7  paramater A to function
PARAM A LSB     8
PARAM B MSB     9  parameter B to function
PARAM B LSB     10
CRC MSB         11 high byte of CRC on everything after STX and before CRC
CRC LSB         12 low byte of CRC 

FUNCTIONS:
0  SEND STATUS PACKET
	no parameters
1  CLEAR MEMORY (LOG_INIT)
    no parameters
	(check for result by checking status packet)
2  REQUEST DATA PAGE
	A: start address (0 to 4096)
    B: number of pages (11 packets per page returned)
   
*/
