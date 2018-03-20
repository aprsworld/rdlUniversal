#include "rdLoggerUniversal.h"

typedef struct {
	/* most recent valid */
	int16 pulse_period;     /* period for #40HC or frequency (Hz) for Thies */
	int16 pulse_min_period; /* period for #40HC or frequency (Hz) for Thies */

	int16 pulse_count_log;  /* reset in log routine */
	int16 pulse_count_live; /* reset in live routine */


	int16 input_voltage_adc;
	int8 wind_direction_sector;
	int8 battery_charge;

	int8 serial_prefix;
	int8 serial_msb;
	int8 serial_lsb;

	int8 compile_year, compile_month, compile_day;
	int16 uptime;
	int8 hardware_type;
	int8 anemometer_type;
	int8 sd_log_rate;
} struct_current;



typedef struct {
	short now_generate_message; 

	int8  buff[128];    
	int16 buff_length;    // bytes of data in buffer
	int16 age;            // age (milliseconds) of receive buffer
	int16 age_response;

	int8  message[129];
	short message_waiting;
} struct_wireless;

typedef struct {
	short now_redraw;
	short now_log;
	short now_log_init;
	short now_log_mark_downloaded;
	short now_live;
	short now_live_status;
	short now_10millisecond;
	short now_second;
	short now_gps_update;

	/* flag to see if we are timing */
	short pulse_0_count;

	short up_now;
	short down_now;
	short select_now;

	int8 port_b;
} struct_action;



typedef struct {
	int8 hour;
	int8 minute;
	int8 second;
	int8 day;
	int8 month;
	int8 year;

	int8 backlight_seconds;	
	int8 modem_seconds;
	int8 live_seconds;

	int16 pulse_period;
	int16 pulse_count; /* used in frequency counting interrupt to count pulses in second */
} struct_time_keep;

typedef struct {
	float latitude;
	float longitude;

	int8 year;
	int8 month;
	int8 day;
	
	int8 hour;
	int8 minute;
	int8 second;
	
	int8 valid;
	int8 rcrc;

	/* raw unparsed ASCII data, null terminated */
	int8 buff[80];
	int8 buff_rcrc[3];
} struct_gps_data;

typedef struct {
	int8  buff_page[528];
	int16 page_requested;
	int16 n_pages;
} struct_log;

/* global structures */
struct_current current;
struct_wireless wireless;
struct_action action;
struct_time_keep timers;
struct_gps_data gps;
struct_log log;

#byte port_a=0x0f80
#byte port_b=0x0f81
#byte port_c=0x0f82
#byte port_d=0x0f83
#byte port_e=0x0f84
#byte port_f=0x0f85
#byte port_g=0x0f86



int16 crc_chk(int8 *data, int8 length) {
	int8 j;
	int16 reg_crc=0xFFFF;

	while ( length-- ) {
		reg_crc ^= *data++;

		for ( j=0 ; j<8 ; j++ ) {
			if ( reg_crc & 0x01 ) {
				reg_crc=(reg_crc>>1) ^ 0xA001;
			} else {
				reg_crc=reg_crc>>1;
			}
		}	
	}
	
	return reg_crc;
}

/* declare routine called by timer0 ISR */
void task_10millisecond(void);

#include "anemometer_thies.c"
#include "interrupts.c"
#include "wireless.c"
#include "lcd.c"
#include "dataflash_at45db161d.c"
#include "fram_fm24c512.c"
#include "log.c"
#include "adc.c"
#include "rtc.c"
#include "screens.c"
#include "gps.c"
#include "live.c"


int16 read_eeprom_int16(int16 address) {
	return make16(read_eeprom(address),read_eeprom(address+1));
}
void write_eeprom_int16(int16 address, int16 val) {
	write_eeprom(address,make8(val,1));
	write_eeprom(address+1,make8(val,0));
}


void basicInit() {
	int8 buff[32];

	setup_oscillator(OSC_8MHZ|OSC_INTRC);
	setup_adc_ports(AN0_TO_AN3 | VSS_VDD);
	setup_adc(ADC_CLOCK_INTERNAL);

	action.now_log=0;
	action.now_log_init=0;
	action.now_log_mark_downloaded=0;
	action.now_redraw=1;
	action.now_live=0;
	action.now_live_status=0;
	action.now_gps_update=1;
	action.now_10millisecond=0;
	action.now_second=0;

	action.up_now=0;
	action.down_now=0;
	action.select_now=0;

	action.port_b=port_b;

	timers.backlight_seconds=BACKLIGHT_TIMEOUT_SECONDS;

	current.pulse_period=0;
	current.pulse_min_period=65535;
	current.pulse_count_live=0;
	current.pulse_count_log=0;

	timers.pulse_count=0;


	/* get our compiled date from constant */
	strcpy(buff,__DATE__);
	current.compile_day =(buff[0]-'0')*10;
	current.compile_day+=(buff[1]-'0');
	/* determine month ... how annoying */
	if ( 'J'==buff[3] ) {
		if ( 'A'==buff[4] )
			current.compile_month=1;
		else if ( 'N'==buff[5] )
			current.compile_month=6;
		else
			current.compile_month=7;
	} else if ( 'A'==buff[3] ) {
		if ( 'P'==buff[4] )
			current.compile_month=4;
		else
			current.compile_month=8;
	} else if ( 'M'==buff[3] ) {
		if ( 'R'==buff[5] )
			current.compile_month=3;
		else
			current.compile_month=5;
	} else if ( 'F'==buff[3] ) {
		current.compile_month=2;
	} else if ( 'S'==buff[3] ) {
		current.compile_month=9;
	} else if ( 'O'==buff[3] ) {
		current.compile_month=10;
	} else if ( 'N'==buff[3] ) {
		current.compile_month=11;
	} else if ( 'D'==buff[3] ) {
		current.compile_month=12;
	} else {
		/* error parsing, shouldn't happen */
		current.compile_month=255;
	}
	current.compile_year =(buff[7]-'0')*10;
	current.compile_year+=(buff[8]-'0');
	current.uptime=0;

	wireless.now_generate_message=0;
	wireless.buff_length=0;
	wireless.age=65535;
	wireless.age_response=65535;
	wireless.message_waiting=0;

	current.serial_prefix=read_eeprom(EE_SERIAL_PREFIX);
	current.serial_msb=read_eeprom(EE_SERIAL_MSB);
	current.serial_lsb=read_eeprom(EE_SERIAL_LSB);

	current.hardware_type=read_eeprom(EE_HW_TYPE);
	current.anemometer_type=read_eeprom(EE_ANEMOMETER_TYPE);
	current.sd_log_rate=read_eeprom(EE_SD_LOG_RATE);

	log.page_requested=65535;


	/* initialize other devices */
	dataflash_init();
	init_ext_fram();
	lcd_init();

	init_rtc();
	if ( bcd2bin(read_rtc(0x06)) < 5 ) {
		reset_rtc();
	}


	/* one periodic interrupt @ 100uS. Generated from system 8 MHz clock */
	/* prescale=4, match=49, postscale=1. Match is 49 because when match occurs, one cycle is lost */
	setup_timer_2(T2_DIV_BY_4,49,1); 


	/* setup timer 3 to be a 10ms timer. It only runs while we are booting */
	setup_timer_3(T3_INTERNAL | T3_DIV_BY_1 ); /* preload to 45536 to get 10.0 milliseconds */

	/* receive data from serial ports */
	enable_interrupts(INT_RDA);
	enable_interrupts(INT_RDA2);

	port_b_pullups(TRUE);
	enable_interrupts(GLOBAL);
}




void serialNumberCheck(void) {
	/* 	middle button sets serial number */
	if ( 'R' != read_eeprom(EE_SERIAL_PREFIX) || read_eeprom(EE_HW_TYPE) > HARDWARE_TYPE_RDLOGGERUNIVERSAL || read_eeprom(EE_ANEMOMETER_TYPE) > ANEMOMETER_TYPE_THIES ) {
		screen_set_serial(1);
	} else if ( 0==action.up_now && 1==action.select_now && 0==action.down_now ) {
		action.select_now=0;
		screen_set_serial(0);
	} 
}

void startupCountdown() {
	int8 i=0;
	int16 serial;

	wirelessOn(20);

	if ( HARDWARE_TYPE_RDLOGGER == current.hardware_type )
		printf(lcd_putch,"rdl(U)");
	else if ( HARDWARE_TYPE_RDLOGGERUNIVERSAL == current.hardware_type ) 
		printf(lcd_putch,"rdlUni");
	else
		printf(lcd_putch,"unknown!");	

	if ( ANEMOMETER_TYPE_40HC == current.anemometer_type ) {
		printf(lcd_putch," #40HC");
	} else if ( ANEMOMETER_TYPE_THIES == current.anemometer_type ) {
		printf(lcd_putch," THIES");
	} else {
		printf(lcd_putch," UNKNOWN");
	}

	serial=make16(read_eeprom(EE_SERIAL_MSB),read_eeprom(EE_SERIAL_LSB));

	for ( i=10 ; i>0 ; i-- ) {
		lcd_goto(LCD_LINE_TWO);
		printf(lcd_putch,"Up in %u ID=%c%lu ",i,read_eeprom(EE_SERIAL_PREFIX),serial);
		fprintf(stream_wireless,"# UP IN %u SECONDS\r\n",i);
		delay_ms(1000);
		restart_wdt();
	}
	

	fprintf(stream_wireless,"# rdLoggerUniversal %s (20%02u-%02u-%02u) (serial %c%lu) (hardware %u) (anemometer %u)\r\n",
		__DATE__,
		current.compile_year,
		current.compile_month,
		current.compile_day,
		read_eeprom(EE_SERIAL_PREFIX),
		serial,
		current.hardware_type,
		current.anemometer_type
	);
}

void task_second(void) {
	static int8 last_minute=100;

	update_time_rtc();

	/* control backlight */
	if (  0 == timers.backlight_seconds ) {
		/* auto, and we have finished counting down */
		output_high(LCD_BACKLIGHT);
	} else {
		output_low(LCD_BACKLIGHT);
		timers.backlight_seconds--;
	}

	/* turn modem on if we got anything */
	if (  0 == timers.modem_seconds ) {
		/* auto, and we have finished counting down */
		wirelessOff();
	} else {
		wirelessOn(0);
		timers.modem_seconds--;
	}

	if ( 0 == timers.live_seconds ) {
		action.now_live=1;
		timers.live_seconds=9;
	} else {
		timers.live_seconds--;
	}

	/* check to see if we are on a new minute */
	if ( timers.minute != last_minute ) {
		action.now_log=1;
		last_minute=timers.minute;


		if ( current.uptime<65535 )
			current.uptime++;

		/* GPS starts at 30 minutes after hour, aborts at 45 minutes */
		if ( 30==timers.minute )
			action.now_gps_update=1;
		else if ( 45 == timers.minute ) 
			action.now_gps_update=0;

		/* send status */
		if ( 5==timers.minute || 20==timers.minute || 35==timers.minute || 50==timers.minute ) {
			action.now_live_status=1;
		}
	}

	/* status LEDs on rdLoggerUniversalXTC */
	/* Logging LED shows if SD card is LOGGING */
	if ( ! input(MMC_STATUS_TO_HOST) ) {
		output_high(LED_LOGGING);
	} else {
		output_low(LED_LOGGING);
	}
	/* Wireless LED shows if Wireless modem is ON */
	if ( 0 != timers.modem_seconds ) {
		output_high(LED_WIRELESS);
	} else {
		output_low(LED_WIRELESS);
	}

	/* Anemometer LED shows if pulse count is > 0 */
	if ( current.pulse_count_live > 0 ) {
		output_high(LED_ANEMOMETER);
	} else {
		output_low(LED_ANEMOMETER);
	}


}


void task_10millisecond(void) {
	static int16 b0_state=0;
	static int16 b1_state=0;
	static int16 b2_state=0;
	int8 b;


	b=port_b;	

	/* check to see if we got a falling edge from the RTC square wave */
	if ( ! bit_test(b,4) && bit_test(action.port_b,4)	) {
		action.now_second=1;
	}

	/* draw screen on rising edge of RTC square wave */
	if ( bit_test(b,4) && ! bit_test(action.port_b,4)	) {
		action.now_redraw=1;
	}

	action.port_b=b;

	/* update timers / counters */
	if ( wireless.age < 65535 ) 
		wireless.age++;
	if ( wireless.age_response < 65535 ) 
		wireless.age_response++;

	/* set flags if we have a message ready */
	if ( wireless.buff_length && wireless.age > 5 && wireless.age < 65535 ) 
		wireless.now_generate_message=1;

	/* button must be down for 12 milliseconds */
	b0_state=(b0_state<<1) | !bit_test(action.port_b,BUTTON_0_BIT) | 0xe000;
	if ( b0_state==0xf000) {
		action.down_now=1;
		action.now_redraw=1;
		timers.backlight_seconds=BACKLIGHT_TIMEOUT_SECONDS;
	}
	b1_state=(b1_state<<1) | !bit_test(action.port_b,BUTTON_1_BIT) | 0xe000;
	if ( b1_state==0xf000) {
		action.select_now=1;	
		action.now_redraw=1;
		timers.backlight_seconds=BACKLIGHT_TIMEOUT_SECONDS;
	}

	b2_state=(b2_state<<1) | !bit_test(action.port_b,BUTTON_2_BIT) | 0xe000;
	if ( b2_state==0xf000) {
		action.up_now=1;
		action.now_redraw=1;
		timers.backlight_seconds=BACKLIGHT_TIMEOUT_SECONDS;
	}
}


void main(void) {
	basicInit();
	enable_interrupts(INT_TIMER3);
	output_low(LCD_BACKLIGHT);

	wirelessOn(30);

	delay_ms(500);

	/* start the modem booting */
	lcd_clear();
	serialNumberCheck();

	startupCountdown();

	/* hold down all three buttons to reset the log */
	if ( 0==bit_test(action.port_b,BUTTON_0_BIT) && 0==bit_test(action.port_b,BUTTON_1_BIT) && 0==bit_test(action.port_b,BUTTON_1_BIT) ) {
		action.up_now=0;
		action.down_now=0;
		action.select_now=0;
		lcd_clear();
		lcd_putch("Press middle");
		lcd_goto(LCD_LINE_TWO);
		lcd_putch("button to clear");
		delay_ms(1000);

		action.select_now=0;
		action.up_now=0;
		action.down_now=0;
		for ( ; ; ) {
			if ( action.select_now ) {
				action.select_now=0;
				log_init();
				break;
			}
			/* let watchdog reset us if we don't press select */
		}
	}

	timers.live_seconds=10;	

	delay_ms(100);

	/* stop 10mS boot up timer */
	disable_interrupts(INT_TIMER3);
	output_high(LCD_BACKLIGHT);


	/* start 100uS timer */
	enable_interrupts(INT_TIMER2);

	
	/* main loop */
	for ( ; ; ) {
		restart_wdt();

		if  ( action.now_10millisecond ) {
			action.now_10millisecond=0;
			task_10millisecond();
		}

		if ( action.now_second ) {
			action.now_second=0;
			task_second();
		}

		/* wireless 802.15.4 related stuff */
		if ( wireless.now_generate_message ) {
			wirelessMessageFromBuff();
			wireless.now_generate_message=0;
		}

		wirelessTick();


		/* send a live packet */
		if ( action.now_live ) {
			liveTask();
		}

		/* status packet */
		if ( action.now_live_status ) {
			liveStatusTask();
		}


		/* handle sending requested log pages */
		logSendTick();


		if ( action.now_redraw ) {
			action.now_redraw=0;
			screen_select();
		}

		if ( action.now_log ) {
			action.now_log=0;
			log_now();
		}

		if ( action.now_log_init ) {
			action.now_log_init=0;
			log_init();
		}

	}
}


