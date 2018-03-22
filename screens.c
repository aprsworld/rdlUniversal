void screen_download(void) {
	printf(lcd_putch,"Page Req: %05lu",log.page_requested);
	lcd_goto(LCD_LINE_TWO);
	printf(lcd_putch," N Pages: %05lu",log.n_pages);
}

void screen_uptime(void) {
	printf(lcd_putch,"Unit Uptime:");
	lcd_goto(LCD_LINE_TWO);
	printf(lcd_putch,"%lu minutes",current.uptime);
}

void screen_modem_seconds(void) {
	printf(lcd_putch,"Wireless on:");
	lcd_goto(LCD_LINE_TWO);
	printf(lcd_putch,"%u seconds",timers.modem_seconds);
}


#if 0
void screen_gps_location(void) {
	printf(lcd_putch,"Lat:  %02.4lf",gps.latitude);
	lcd_goto(LCD_LINE_TWO);
	printf(lcd_putch,"Lon: %03.4lf",gps.longitude);

	if ( action.now_gps_update ) {
		lcd_goto(LCD_LINE_ONE+15); lcd_putch('O');
		lcd_goto(LCD_LINE_TWO+15); lcd_putch('N');
	}

	if ( action.select_now ) {
		action.select_now=0;
		action.now_gps_update=1;
	}
}
#endif

void screen_sd(void) {
	printf(lcd_putch,"SD Card Status:");
	lcd_goto(LCD_LINE_TWO);
	if ( ! input(MMC_STATUS_TO_HOST) ) {
		if ( SD_LOG_RATE_10 == current.sd_log_rate ) {
			printf(lcd_putch,"Logging (10 sec)");
		} else {
			printf(lcd_putch,"Logging (60 sec)",);
//                            0123456789012345
		}
	} else {
		if ( SD_LOG_RATE_10 == current.sd_log_rate ) {
			printf(lcd_putch,"Not ready! (10)",);
		} else {
			printf(lcd_putch,"Not ready! (60)",);
//                            0123456789012345
		}
	}


	if ( action.select_now ) {
		action.select_now=0;
		action.now_live_status=1;
	}
}

void screen_dataflash(void) {
	printf(lcd_putch,"Internal Memory:");
	lcd_goto(LCD_LINE_TWO);
	printf(lcd_putch,"%lu/%lu full",make16(read_ext_fram(FRAM_ADDR_DATAFLASH_PAGE+0),read_ext_fram(FRAM_ADDR_DATAFLASH_PAGE+1)),DATAFLASH_PAGES);
}

void screen_power(void) {	
	float voltage;

	voltage=(40.0/1024)*current.input_voltage_adc;
	printf(lcd_putch,"Input: %0.1lf volts",voltage);
	//printf(lcd_putch,"Input: %lu volts",current.input_voltage_adc);
	lcd_goto(LCD_LINE_TWO);
	
	printf(lcd_putch,"Battery: %d%%",current.battery_charge*10);
	
}

void screen_wind_direction(void) {
	printf(lcd_putch,"Wind Direction:");
	lcd_goto(LCD_LINE_TWO);
	printf(lcd_putch,"Sector %u (",current.wind_direction_sector);

	switch ( current.wind_direction_sector ) {
		case 0: lcd_putch('0'); break;
		case 1: lcd_putch("45"); break;
		case 2: lcd_putch("90"); break;
		case 3: lcd_putch("135"); break;
		case 4: lcd_putch("180"); break;
		case 5: lcd_putch("225"); break;
		case 6: lcd_putch("270"); break;
		case 7: lcd_putch("315"); break;
	}

	printf(lcd_putch,"%c)",DEG_SYMBOL);
}

void screen_t(void) {
	if ( ANEMOMETER_TYPE_THIES == current.anemometer_type ) {
		printf(lcd_putch,"%lu",current.pulse_period);
		lcd_goto(LCD_LINE_ONE+6);
		printf(lcd_putch,"%lu",current.pulse_min_period);
		lcd_goto(LCD_LINE_ONE+12);
		printf(lcd_putch,"%lu",current.pulse_count_live);
		
		lcd_goto(LCD_LINE_TWO);
		printf(lcd_putch,"F(Hz) FMax  Cnt");
//                        0123456789012345


	} else {
		printf(lcd_putch,"%05lu",current.pulse_period);
		lcd_goto(LCD_LINE_ONE+6);
		printf(lcd_putch,"%05lu",current.pulse_min_period);
		lcd_goto(LCD_LINE_ONE+12);
		printf(lcd_putch,"%04lu",current.pulse_count_live);
		
		lcd_goto(LCD_LINE_TWO);
		
		printf(lcd_putch,"T     Tmin Count");
	}
}

void screen_time_date(void) {
	printf(lcd_putch,"Date: 20%02u-%02u-%02u",timers.year,timers.month,timers.day);
	lcd_goto(LCD_LINE_TWO);
	printf(lcd_putch,"Now %02u:%02u:%02u UTC",timers.hour,timers.minute,timers.second);
}

void screen_version(void) {
	printf(lcd_putch,"Firmware Version:");
//                    0123456789012345
	lcd_goto(LCD_LINE_TWO);
	printf(lcd_putch,"%s  %s",__DATE__,__TIME__);
}

void screen_wind(void) {
	float f;

	if ( ANEMOMETER_TYPE_40HC == current.anemometer_type ) {
		if ( current.pulse_period>0 && current.pulse_period<65535 ) {
			f = 7650.0 / current.pulse_period + 0.35;
		} else {
			f = 0.0;
		}
		printf(lcd_putch,"Speed: %02.1f m/s",f);

		if ( current.pulse_min_period>0 && current.pulse_min_period<65535 ) {
			f = 7650.0 / current.pulse_min_period + 0.35;
		} else { 
			f = 0.0;
		}

		lcd_goto(LCD_LINE_TWO);
		printf(lcd_putch,"Gust:  %02.1f m/s",f);

	} else if ( ANEMOMETER_TYPE_THIES == current.anemometer_type ) {
		printf(lcd_putch,"Speed: %04.1w m/s",anemometer_thies_to_ws(current.pulse_period));
		lcd_goto(LCD_LINE_TWO);
		printf(lcd_putch,"Gust:  %04.1w m/s",anemometer_thies_to_ws(current.pulse_min_period));
	} 
}

void prompt_prev_set_next(void) {
	lcd_goto(LCD_LINE_TWO);
	printf(lcd_putch,"Prev  Set   Next");
}
void prompt_minus_set_plus(void) {
	lcd_goto(LCD_LINE_TWO);
	printf(lcd_putch,"-     Set      +");
}
void prompt_minus_next_plus(void) {
	lcd_goto(LCD_LINE_TWO);
	printf(lcd_putch,"-     Next     +");
}
void prompt_no_yes(void) {
	lcd_goto(LCD_LINE_TWO);
	printf(lcd_putch,"NO    YES");
}
void prompt_prev_enter_next(void) {
	lcd_goto(LCD_LINE_TWO);
	printf(lcd_putch,"Prev Enter  Next");
}

short screen_get_int16() {
	char n_str[6];

	sprintf(n_str,"%05lu",keypad.value);

//	printf("current position value=%u field=%u\r\n",n_str[keypad.field]-'0',keypad.field);

	/* print bottom line */
	prompt_minus_set_plus();


	/* print our value */
	lcd_goto(keypad.start_pos);
	printf(lcd_putch,"%s",n_str);
	/* put cursor on field being changed */
	lcd_goto(keypad.field + keypad.start_pos);
	lcd_cursor(LCD_CURSOR_ON);
	

	if ( 1 == action.up_now ) {
		action.up_now=0;

		if ( '5'==n_str[keypad.field] && 0==keypad.field ) 
			keypad.value -= 50000;
		else if ( '9' == n_str[keypad.field] )
			keypad.value -= int16_tens[keypad.field]*9;
		else
			keypad.value += int16_tens[keypad.field];

		action.now_redraw=1;
	}
	if ( 1 == action.down_now ) {
		action.down_now=0;
		if ( '0'==n_str[keypad.field] && 0==keypad.field ) 
			keypad.value += 50000;
		else if ( '0' == n_str[keypad.field] ) 
			keypad.value += int16_tens[keypad.field]*9;
		else	
			keypad.value -= int16_tens[keypad.field];

		action.now_redraw=1;
	}
	if ( 1 == action.select_now ) {
		action.select_now=0;
		keypad.field++;
		action.now_redraw=1;
	}

	if ( 5 == keypad.field ) {
		lcd_cursor(LCD_CURSOR_OFF);
		return 1;
	}

	return 0;
}

void screen_set_serial(short reset) {
	int8 serial_prefix;
	int16 serial;
	int8 hwtype;
	int8 antype;
	int8 sdrate;
	int16 last_serial_number;

	if ( reset ) {
		write_eeprom(EE_SERIAL_PREFIX,'R');
		write_eeprom(EE_SERIAL_MSB,0x00);
		write_eeprom(EE_SERIAL_LSB,0x00);
		write_eeprom(EE_HW_TYPE,HARDWARE_TYPE_RDLOGGERUNIVERSAL);
		write_eeprom(EE_ANEMOMETER_TYPE,ANEMOMETER_TYPE_40HC);
		write_eeprom(EE_SD_LOG_RATE,SD_LOG_RATE_60);


		lcd_clear();
		lcd_putch("Wrote Default SN");
		delay_ms(1000);
		lcd_clear();
	}
	
	serial_prefix=read_eeprom(EE_SERIAL_PREFIX);
	serial=make16(read_eeprom(EE_SERIAL_MSB),read_eeprom(EE_SERIAL_LSB));

	action.select_now=action.up_now=action.down_now=0;

#if 1
	/* from sdLoggerThermok */
	lcd_clear();

	printf(lcd_putch,"Setting SN...");
	delay_ms(2500);

	/* set our values for next state */
	keypad.value=serial; 
	keypad.field=0;
	keypad.start_pos=9;

	last_serial_number=65535;

#ignore_warnings 203
	while ( 1 ) {
#ignore_warnings NONE
		restart_wdt();

		if ( last_serial_number != keypad.value ) {
			lcd_clear();
			printf(lcd_putch,"Serial: %c",serial_prefix);
			last_serial_number=keypad.value;
		}

		if ( screen_get_int16() ) {
			serial=keypad.value;
			break;
		}
	}

#else
	/* old style press press press press */
	for ( ; ; ) {
		lcd_goto(LCD_LINE_ONE);
		printf(lcd_putch,"Serial: %c%lu     ",serial_prefix,serial);
		lcd_goto(LCD_LINE_TWO);
		printf(lcd_putch,"-     DONE     +");


		if ( action.select_now ) {
			action.select_now=0;
			break;
		}

		if ( action.up_now ) {
			action.up_now=0;
			serial++;
		}

		if ( action.down_now ) {
			action.down_now=0;
			serial--;
		}

		delay_ms(20);
		restart_wdt();
	}
#endif


	/* write the new serial number */
	write_eeprom(EE_SERIAL_PREFIX,'R');
	write_eeprom(EE_SERIAL_MSB,make8(serial,1));
	write_eeprom(EE_SERIAL_LSB,make8(serial,0));
	write_eeprom(EE_HW_TYPE,hwtype);
	write_eeprom(EE_ANEMOMETER_TYPE,antype);

	/* read it back out */
	current.serial_prefix=read_eeprom(EE_SERIAL_PREFIX);
	current.serial_msb=read_eeprom(EE_SERIAL_MSB);
	current.serial_lsb=read_eeprom(EE_SERIAL_LSB);
	current.hardware_type=read_eeprom(EE_HW_TYPE);
	current.anemometer_type=read_eeprom(EE_ANEMOMETER_TYPE);


	lcd_clear();
	lcd_putch("Wrote SN");
	delay_ms(1000);
	lcd_clear();


	for ( ; ; ) {

		lcd_goto(LCD_LINE_ONE);
		printf(lcd_putch," - HW TYPE - ");
		lcd_goto(LCD_LINE_TWO);
		printf(lcd_putch,"RDL         RDLU");

		if ( action.select_now ) {
			action.select_now=0;
		}

		if ( action.up_now ) {
			action.up_now=0;
			hwtype=HARDWARE_TYPE_RDLOGGERUNIVERSAL;
			break;
		}

		if ( action.down_now ) {
			action.down_now=0;
			hwtype=HARDWARE_TYPE_RDLOGGER;
			break;
		}

		delay_ms(20);
		restart_wdt();
	}


	/* write the new hardware type */
	write_eeprom(EE_HW_TYPE,hwtype);

	/* read it back out */
	current.hardware_type=read_eeprom(EE_HW_TYPE);


	lcd_clear();
	lcd_putch("Wrote HW Type");
	delay_ms(1000);
	lcd_clear();

	for ( ; ; ) {

		lcd_goto(LCD_LINE_ONE);
		printf(lcd_putch," - ANEMO TYPE - ");
		lcd_goto(LCD_LINE_TWO);
		printf(lcd_putch,"#40HC      THIES");
//                        0123456789012345

		if ( action.select_now ) {
			action.select_now=0;
		}

		if ( action.up_now ) {
			action.up_now=0;
			antype=ANEMOMETER_TYPE_THIES;
			break;
		}

		if ( action.down_now ) {
			action.down_now=0;
			antype=ANEMOMETER_TYPE_40HC;
			break;
		}

		delay_ms(20);
		restart_wdt();
	}


	/* write the new hardware type */
	write_eeprom(EE_ANEMOMETER_TYPE,antype);

	/* read it back out */
	current.anemometer_type=read_eeprom(EE_ANEMOMETER_TYPE);


	lcd_clear();
	lcd_putch("Wrote Anemo Type");
	delay_ms(1000);
	lcd_clear();



	/* SD card log interval */
	for ( ; ; ) {

		lcd_goto(LCD_LINE_ONE);
		printf(lcd_putch," - SD LOG RATE - ");
		lcd_goto(LCD_LINE_TWO);
		printf(lcd_putch,"60 sec    10 sec");
//                        0123456789012345

		if ( action.select_now ) {
			action.select_now=0;
		}

		if ( action.up_now ) {
			action.up_now=0;
			sdrate=SD_LOG_RATE_10;
			break;
		}

		if ( action.down_now ) {
			action.down_now=0;
			sdrate=SD_LOG_RATE_60;
			break;
		}

		delay_ms(20);
		restart_wdt();
	}


	/* write the new hardware type */
	write_eeprom(EE_SD_LOG_RATE,sdrate);

	/* read it back out */
	current.sd_log_rate=read_eeprom(EE_SD_LOG_RATE);


	lcd_clear();
	lcd_putch("Wrote SD rate");
	delay_ms(1000);
	lcd_clear();

}


short screen_set_date(void) {
	static short got_buttons=0;
	static int field=0;
	int spos;
	int min,max;
	signed int value;
	short save=0;

	switch ( field ) {
		case 0:
			/* year */
			min=0;
			max=99;
			spos=9;
			value=timers.year;
			break;
		case 1:
			/* month */
			min=1;
			max=12;
			spos=12;
			value=timers.month;
			break;
		case 2:
			/* day */
			min=1;
			max=31;
			spos=15;
			value=timers.day;
			break;
	}

	printf(lcd_putch,"Date: 20%02d-%02d-%02d",timers.year,timers.month,timers.day);


	if ( 0 == got_buttons ) {
		if ( 1 == action.select_now ) {
			/* set button pressed */
			got_buttons=1;
			action.select_now=0;
		} else {
			/* prompt to set */
			prompt_prev_set_next();
		}
	} 

	if ( 1 == got_buttons ) {
		prompt_minus_set_plus();
		lcd_goto(spos);
		lcd_cursor(LCD_CURSOR_ON);

		if ( 1 == action.up_now ) {
			action.up_now=0;
			value++;
			if ( value > max )
				value=min;
			save=1;
		}
		if ( 1 == action.down_now ) {
			action.down_now=0;
			value--;
			if ( value < min )
				value=max;
			save=1;
		}
		if ( 1 == action.select_now ) {
			action.select_now=0;
			field++;
		}
	}
	
	if ( save ) {
		/* redraw our screen ASAP */
		action.now_redraw=1;

		switch ( field ) {
			case 0:
				/* write year */
				timers.year=value;
				write_rtc(0x06,bin2bcd(timers.year));
//				puts("# writing year");
				break;
			case 1:
				/* write month */
				timers.month=value;
				write_rtc(0x05,bin2bcd(timers.month));
//				puts("# writing month");
				break;
			case 2:
				/* write day */
				timers.day=value;
				write_rtc(0x04,bin2bcd(timers.day));
//				puts("# writing day");
				break;
		}
	}


	if ( 3 == field ) {
//		puts("# done");
		got_buttons=0;
		field=0;
		lcd_cursor(LCD_CURSOR_OFF);
	}


	return got_buttons;
}

short screen_set_time(void) {
	static short got_buttons=0;
	static int field=0;
	int spos;
	int min,max;
	signed int value;
	short save=0;

	switch ( field ) {
		case 0:
			/* hour */
			min=0;
			max=23;
			spos=7;
			value=timers.hour;
			break;
		case 1:
			/* minute */
			min=0;
			max=59;
			spos=10;
			value=timers.minute;
			break;
		case 2:
			/* second */
			min=0;
			max=59;
			spos=13;
			value=timers.second;
			break;
	}

	printf(lcd_putch,"Time: %02d:%02d:%02d",timers.hour,timers.minute,timers.second);


	if ( 0 == got_buttons ) {
		if ( action.select_now ) {
			/* set button pressed */
			got_buttons=1;
			action.select_now=0;
			action.now_redraw=1;
		} else {
			/* prompt to set */
			prompt_prev_set_next();
		}
	} 

	if ( 1 == got_buttons ) {
		prompt_minus_set_plus();
		lcd_goto(spos);
		lcd_cursor(LCD_CURSOR_ON);

		if ( action.up_now ) {
			action.up_now=0;
			value++;
			if ( value > max )
				value=min;
			save=1;
			action.now_redraw=1;
		}
		if ( action.down_now ) {
			action.down_now=0;
			value--;
			if ( value < min )
				value=max;
			save=1;
			action.now_redraw=1;
		}
		if ( action.select_now ) {
			action.select_now=0;
			field++;
			action.now_redraw=1;
		}
	}
	
	if ( save ) {
		/* redraw our screen ASAP */
		action.now_redraw=1;

		switch ( field ) {
			case 0:
				/* write hour */
				timers.hour=value;
				write_rtc(0x02,bin2bcd(timers.hour));
//				puts("writing hour");
				break;
			case 1:
				/* write minute */
				timers.minute=value;
				write_rtc(0x01,bin2bcd(timers.minute));
//				puts("writing minute");
				break;
			case 2:
				/* write second */
				timers.second=value;
				write_rtc(0x00,0b01111111 & bin2bcd(timers.second));
//				puts("writing second");
				break;
		}
	}

	/* done with all fields */
	if ( 3 == field ) {
		got_buttons=0;
		field=0;
		lcd_cursor(LCD_CURSOR_OFF);
		action.now_redraw=1;
	}
//	action.redraw_next=1;

	return got_buttons;
}



#define MAX_SCREEN_RDLOGGER     11
void screen_select(void) {
	static int8 screen;
	static short has_buttons=0;
	int8 max_screen;

	max_screen=MAX_SCREEN_RDLOGGER;	

	if ( ! has_buttons && action.up_now ) {
		action.up_now=0;

		if ( screen<max_screen ) {
			screen++;
		} else {
			screen=0;
		}
	}

	if ( ! has_buttons && action.down_now ) {
		action.down_now=0;

		if ( screen != 0 ) {
			screen--;
		} else {
			screen=max_screen;
		}
	}

	lcd_clear();


		switch ( screen ) {
			case 0:  screen_wind(); has_buttons=0; break;
			case 1:  screen_wind_direction(); has_buttons=0; break;
			case 2:  screen_time_date(); has_buttons=0; break;
			case 3:  screen_dataflash(); has_buttons=0; break;
			case 4:  screen_sd(); has_buttons=0; break;
			case 5:  screen_t(); has_buttons=0; break;
			case 6:  screen_power(); has_buttons=0; break;
			case 7:  screen_modem_seconds(); has_buttons=0; break;
			case 8:  has_buttons=screen_set_date();  break;
			case 9:  has_buttons=screen_set_time(); break;
			case 10: screen_download(); has_buttons=0; break;
			case 11: screen_version(); has_buttons=0; break;
		}

}