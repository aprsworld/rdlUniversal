#define RECORD_LENGTH 12
#define RECORDS_PER_DATAFLASH_BLOCK 44
#define RECORD_INTER_DELAY 5


#define FRAM_ADDR_RECORD_N       0 
#define FRAM_ADDR_DATAFLASH_PAGE 2L
#define FRAM_ADDR_BUFFER_START   512L
#define FRAM_ADDR_BUFFER_END     1040L

void log_init(void) {
	fprintf(stream_wireless,"# initializing logging\r\n");
	lcd_clear();
	printf(lcd_putch,"Resetting Log!");
	delay_ms(1200);
	write_ext_fram(FRAM_ADDR_RECORD_N,0);
	write_ext_fram(FRAM_ADDR_DATAFLASH_PAGE+0,0);
	write_ext_fram(FRAM_ADDR_DATAFLASH_PAGE+1,0);

	/* erase the first block of the data flash */
	dataflash_select();
	spi_write2(0x50);
	spi_write2(0x00);
	spi_write2(0x00);
	spi_write2(0x00); 
	dataflash_unselect();
	delay_ms(10);
	
	/* wait for page to finish erasing and writing */
	while ( ! bit_test(dataflash_read_status(),7) ) {
		restart_wdt();
		//fputc('x',modem);
	}

	lcd_goto(LCD_LINE_TWO);
	printf(lcd_putch,"Done");
	fprintf(stream_wireless,"> done\r\n");
	delay_ms(1200);
}

void record_print(int8 *record_buffer) {
	/* decode and print */
	/* date, pulseTime, pulseMinTime, pulseCount, batteryPercentCharged, wind direction sector */
	fprintf(stream_wireless,"20%02u-%02u-%02u %02u:%02u,%lu,%lu,%lu,%u,%u\r\n",\
		record_buffer[0],\
		record_buffer[1],\
		record_buffer[2],\
		record_buffer[3],\
		record_buffer[4],\
		make16(record_buffer[5],record_buffer[6]),\
		make16(record_buffer[7],record_buffer[8]),\
		make16(record_buffer[9],record_buffer[10]),\
		(record_buffer[11]>>4)*10,\
		(record_buffer[11]&0x0F)\
	);

}

void log_play(void) {
	int8 record_buffer[RECORD_LENGTH];
	int8 i;
	int16 page,address,l;
	short terminated=0;
	int16 dataflash_max_page;

	dataflash_max_page=make16(read_ext_fram(FRAM_ADDR_DATAFLASH_PAGE+0),read_ext_fram(FRAM_ADDR_DATAFLASH_PAGE+1));
	

	for ( i=0 ; i<RECORD_LENGTH ; i++ ) {
		record_buffer[i]=0;
	}

	fprintf(stream_wireless,"# playing log\r\n");

	dataflash_select();
	spi_write2(0x03);
	/* start on page 0 */
	spi_write2(0x00);
	spi_write2(0x00);
	spi_write2(0x00); 

	/* read data until we reach a record with first three bytes all 0xFF */
	for ( page=0 ; page<DATAFLASH_PAGES && page<=dataflash_max_page ; page++ ) {
		restart_wdt();
		wirelessOn(5);
		for ( address=0 ; address<DATAFLASH_BYTES_PER_PAGE ; address+=RECORD_LENGTH ) {
			for ( i=0 ; i<RECORD_LENGTH ; i++ ) {
				record_buffer[i]=spi_read2(0x00);
			}
			
			/* check for terminator record */
			if ( 0xFF==record_buffer[0] && 0xFF==record_buffer[1] && 0xFF==record_buffer[2] ) {
//				fprintf(modem,"# terminator found at page=%lu record_start_address=%lu\r\n",page,address);
				terminated=1;				
				break;
			}
			/* send the record to the modem */
			record_print(record_buffer);
#if RECORD_INTER_DELAY
			/* give the modem a chance to catch up */
			delay_ms(RECORD_INTER_DELAY);
#endif
		}

		if ( terminated ) break;
	}
	dataflash_unselect();

//	if ( ! terminated ) {
//		fprintf(modem,"# no terminator record found!\r\n");
//	}

//	fprintf(modem,"# dumping records remaining in FRAM:\r\n");
	/* dump extra records still in FRAM */
	address = FRAM_ADDR_BUFFER_START + ( (int16) RECORD_LENGTH * (int16) read_ext_fram(FRAM_ADDR_RECORD_N) );
	for ( l=FRAM_ADDR_BUFFER_START ; l<address ; l+=RECORD_LENGTH ) {
		restart_wdt();
		wirelessOn(5);
		for ( i=0 ; i<RECORD_LENGTH ; i++ ) {
			record_buffer[i]=read_ext_fram(l+i);
		}
		record_print(record_buffer);
#if RECORD_INTER_DELAY		
		/* give the modem a chance to catch up */
		delay_ms(RECORD_INTER_DELAY);
#endif
	}

	fprintf(stream_wireless,"> done\r\n");
}

void write_record(int8 *record) {
	int16 address;
	int8 record_number;
	int16 dataflash_page;
	int8 i;

	/* lookup location to buffer in fram */
	record_number = read_ext_fram(FRAM_ADDR_RECORD_N);
//	fprintf(stream_wireless,"# record n from fram = %u of %u\r\n",record_number,RECORDS_PER_DATAFLASH_BLOCK);

	address = FRAM_ADDR_BUFFER_START + ( (int16) RECORD_LENGTH *  (int16) record_number);
//	fprintf(stream_wireless,"# write_record to FRAM address=%lu\r\n",address);

	/* write record_length bytes to that location in FRAM */
	for ( i=0 ; i<RECORD_LENGTH ; i++ ) {
		write_ext_fram(address+i,record[i]);
	}
	/* update record number */
	record_number++;
	write_ext_fram(FRAM_ADDR_RECORD_N,record_number);

	/* check see if we have hit RECORDS_PER_DATAFLASH_BLOCK and
	write to dataflash if we need to */
	if ( record_number == RECORDS_PER_DATAFLASH_BLOCK ) {
//		fprintf(stream_wireless,"# FRAM buffer / dataflash page full. Will write.\r\n");
		dataflash_page = make16(read_ext_fram(FRAM_ADDR_DATAFLASH_PAGE+0),read_ext_fram(FRAM_ADDR_DATAFLASH_PAGE+1));
//		fprintf(stream_wireless,"# dataflash_page=%lu of %lu\r\n",dataflash_page,DATAFLASH_PAGES);

		/* read data block from FRAM and write to data flash */
		dataflash_resume();
		dataflash_select();
		spi_write2(0x82);
		address = dataflash_page << 2;
		spi_write2(make8(address,1));
		spi_write2(make8(address,0));
		spi_write2(0);

		for ( address=0 ; address<DATAFLASH_BYTES_PER_PAGE ; address++ ) {
			restart_wdt();
			i = read_ext_fram(address+FRAM_ADDR_BUFFER_START);
			spi_write2(i);
		}
		dataflash_unselect();

		/* update locations in FRAM */
		write_ext_fram(FRAM_ADDR_RECORD_N,0);
		dataflash_page++;
		write_ext_fram(FRAM_ADDR_DATAFLASH_PAGE+0,make8(dataflash_page,1));
		write_ext_fram(FRAM_ADDR_DATAFLASH_PAGE+1,make8(dataflash_page,0));
	}
}

void log_now(void) {
	int8 buff_binary[RECORD_LENGTH];
//	int8 buff_hex[64]; /* to data flash */
	int8 buff_decimal[256]; /* to SD card and debugging */
	int16 pulse_period,pulse_min_period,pulse_count;
	int8 i;
	int16 l;

	/* shut off main timer so we don't change while we copy */
	disable_interrupts(INT_TIMER2);
	pulse_period = current.pulse_period;
	pulse_min_period = current.pulse_min_period;
	pulse_count = current.pulse_count_log;

	/* reset our current record */
	current.pulse_period=0;
	current.pulse_min_period=0xffff;
	current.pulse_count_log=0;
	enable_interrupts(INT_TIMER2);

	/* binary */
	buff_binary[0]=timers.year;
	buff_binary[1]=timers.month;
	buff_binary[2]=timers.day;
	buff_binary[3]=timers.hour;
	buff_binary[4]=timers.minute;


	if ( ANEMOMETER_TYPE_THIES == current.anemometer_type ) {
		/* scale Thies anemometer frequency to #40HC recipricol frequency */
		l=anemometer_thies_to_40HC(pulse_period);
		buff_binary[5]=make8(l,1); /* wind speed */
		buff_binary[6]=make8(l,0);

		l=anemometer_thies_to_40HC(pulse_min_period);
		buff_binary[7]=make8(l,1); /* wind gust */
		buff_binary[8]=make8(l,0);

	} else {
		buff_binary[5]=make8(pulse_period,1);
		buff_binary[6]=make8(pulse_period,0);
		buff_binary[7]=make8(pulse_min_period,1);
		buff_binary[8]=make8(pulse_min_period,0);
	}

	buff_binary[9]=make8(pulse_count,1);
	buff_binary[10]=make8(pulse_count,0);
	buff_binary[11]=(current.battery_charge<<4) + (current.wind_direction_sector & 0x0F); /* battery % full, wind direction sector */
	
	/* debugging */
//	fprintf(modem,"# logging  internal: %s\r\n",buff_hex);
//	fprintf(modem,"#       test binary: ");
//	for ( i=0 ; i<RECORD_LENGTH ; i++ ) {
//		fprintf(modem,"%02x",buff_binary[i]);
//	}
//	fprintf(modem,"\r\n");
//	fprintf(modem,"# logging dataflash: %s\r\n",buff_decimal);

	/* write to dataflash */
	write_record(buff_binary);


	if ( SD_LOG_RATE_10 != current.sd_log_rate ) {
		/* write to SD card */
		/* decimal for human readability and for mmcDaughter */
		sprintf(buff_decimal,"20%02u-%02u-%02u %02u:%02u,%lu,%lu,%lu,%lu,%u,%lu,%c%lu\n",
			timers.year, 
			timers.month, 
			timers.day, 
			timers.hour, 
			timers.minute, 
			pulse_period, 
			pulse_min_period, 
			pulse_count, 
			current.input_voltage_adc,
			current.wind_direction_sector,
			current.uptime,
			current.serial_prefix,
			make16(current.serial_msb,current.serial_lsb)
		);

		for ( i=0 ; i<strlen(buff_decimal) ; i++ ) {
			/* rdLogger via builtin UART2 */
			fputc(buff_decimal[i],stream_sd);
			delay_ms(1);
		}
	}

}


void log_dump(void) {
	int16 l;
	fprintf(stream_wireless,"# Dumping log\r\n");

	for ( l=0 ; l<DATAFLASH_PAGES ; l++ ) {
		restart_wdt();
		wirelessOn(5);
		dataflash_page_read(l);
	}

	fprintf(stream_wireless,"> done\r\n");

}

/* a packet is 4 records (48 bytes) + headers + crc */
void log_send_packet(int16 packetNumber, int16 buff_offset) {
	int8 buff[58];
	int16 lCRC;
	int8 i;

	buff[0]='#';
	buff[1]=current.serial_prefix;
	buff[2]=current.serial_msb;
	buff[3]=current.serial_lsb;
	buff[4]=sizeof(buff); /* packet length */
	buff[5]=21; /* packet type */

	buff[6]=make8(packetNumber,1);
	buff[7]=make8(packetNumber,0);

	for ( i=0 ; i<48 ; i++ ) {
		buff[8+i]=log.buff_page[buff_offset+i];
	}

	lCRC=crc_chk(buff+1,55);
	buff[56]=make8(lCRC,1);
	buff[57]=make8(lCRC,0);

	/* about 10 milliseconds to send all of this */
	for ( i=0 ; i<sizeof(buff) ; i++ ) {
		/* xbee modem */
		fputc(buff[i],stream_wireless);
	}	
}

void log_buff_page_dataflash(int16 page) {
	int16 l;

	/* read a page of dataflash into RAM */
	dataflash_select();
	spi_write2(0xD2);
	page = page << 2;
	spi_write2(make8(page,1));
	spi_write2(make8(page,0));
	spi_write2(0x00); 

	/* four don't care bytes */
	spi_write2(0x00);
	spi_write2(0x00);
	spi_write2(0x00);
	spi_write2(0x00);

	/* start reading */
	for ( l=0 ; l< (int16) DATAFLASH_BYTES_PER_PAGE ; l++ ) {
		log.buff_page[l]=spi_read2(0x00);
	}
	dataflash_unselect();
}

void log_buff_page_fram(void) {
	int16 l;
	int16 stop;

	stop = (int16) RECORD_LENGTH * (int16) read_ext_fram(FRAM_ADDR_RECORD_N);

//	for ( l=0 ; l< (int16) DATAFLASH_BYTES_PER_PAGE ; l++ ) {

	/* send all of the data we have */
	for ( l=0 ; l<stop ; l++ ) {
		log.buff_page[l]=read_ext_fram(l+ (int16) FRAM_ADDR_BUFFER_START);
	}
	/* then send terminator records to round things out */
	for ( ; l < (int16) DATAFLASH_BYTES_PER_PAGE ; l++ ) {
		log.buff_page[l]=0xff;
	}
}

/* just set log.page_requested, and this tick process will 
send the page */
void logSendTick(void) {
	static int8 packet=0;

	if ( log.page_requested > (DATAFLASH_PAGES+1 ) || 0==log.n_pages ) {
		/* nothing to do */
		log.n_pages=0;
		return;
	}


	if ( input(MODEM_CTS) ) {
		/* wireless modem is full, try next time */
		return;
	}

	if ( 0==packet ) {
//		output_high(AN1_FILTERED);
		/* got a request for a new page */
		if ( log.page_requested < DATAFLASH_PAGES ) {
			log_buff_page_dataflash(log.page_requested);
		} else {
			log_buff_page_fram();
		}
//		output_low(AN1_FILTERED);
	}

	/* send the actual packet */
	wirelessOn(1);
	log_send_packet(log.page_requested*11+packet, ((int16) packet)*48); /* the int16 cast is important, for some reason! */
	packet++;

	if ( 11 == packet ) {
		/* done with this page */
		packet=0;

		if ( log.n_pages > 0 ) {
			log.page_requested++;
			log.n_pages--;
		} else {
			log.page_requested=65535;
		}
	}
//	output_low(AN1_FILTERED);
}