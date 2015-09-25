#include "rdLoggerUniversal.h"

typedef struct {
	/* most recent valid */
	int16 pulse_period;
	int16 pulse_min_period;
	int16 pulse_count;

	int16 input_voltage_adc;
	int8 wind_direction_sector;
	int8 battery_charge;

	int8 serial_prefix;
	int8 serial_msb;
	int8 serial_lsb;

	int8 compile_year, compile_month, compile_day;
	int16 uptime;
	int8 hardware_type;
} struct_current;

#define GPRS_STATE_DISCONNECTED 0
#define GPRS_STATE_READY        1
#define GPRS_STATE_ERROR        2

typedef struct {
	int8  state;
	int8  connect_state;
	int8  connect_retries;
	int8  connects_failed;

	short now_generate_message; 
	short connection_open;
	int16 uptime;


	int8  buff[512];    
	int16 buff_length;    // bytes of data in buffer
	int16 age;            // age (milliseconds) of receive buffer
	int16 age_response;

	int8  message[513];
	short message_waiting;

	int8  checksum_last_msb, checksum_last_lsb;
	int8  checksum_rx_msb, checksum_rx_lsb;
	short checksum_rx_used;
	int8  missed_acks;
} struct_gprs;

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
//	short now_log_dump;
	short now_log_init;
	short now_log_mark_downloaded;
//	short now_log_play;
	short now_live;
	short now_live_status;
//	short now_debug;
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
	int16 pulse_count;
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
struct_gprs gprs;
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

#include "interrupts.c"
#include "wireless.c"
#include "gprs.c"
#include "lcd.c"
#include "dataflash_at45db161d.c"
#include "fram_fm24c512.c"
#include "uart_sci16is740.c"
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
	setup_oscillator(OSC_8MHZ|OSC_INTRC);
//	if ( HARDWARE_TYPE_RDLOGGERCELL == current.hardware_type )  {
//		setup_adc_ports(AN0_TO_AN1 | VSS_VDD);
//	} else {
		setup_adc_ports(AN0_TO_AN3 | VSS_VDD);
//	}
	setup_adc(ADC_CLOCK_INTERNAL);

	action.now_log=0;
	action.now_log_init=0;
	action.now_log_mark_downloaded=0;
	action.now_redraw=1;
	action.now_live=0;
	action.now_live_status=0;
	action.now_gps_update=1;
	action.now_second=0;

	action.up_now=0;
	action.down_now=0;
	action.select_now=0;

	action.port_b=port_b;

	timers.backlight_seconds=BACKLIGHT_TIMEOUT_SECONDS;

	current.pulse_period=0;
	current.pulse_min_period=65535;
	current.pulse_count=0;

	/* get our compiled date from constant */
	strcpy(gprs.buff,__DATE__);
	current.compile_day =(gprs.buff[0]-'0')*10;
	current.compile_day+=(gprs.buff[1]-'0');
	/* determine month ... how annoying */
	if ( 'J'==gprs.buff[3] ) {
		if ( 'A'==gprs.buff[4] )
			current.compile_month=1;
		else if ( 'N'==gprs.buff[5] )
			current.compile_month=6;
		else
			current.compile_month=7;
	} else if ( 'A'==gprs.buff[3] ) {
		if ( 'P'==gprs.buff[4] )
			current.compile_month=4;
		else
			current.compile_month=8;
	} else if ( 'M'==gprs.buff[3] ) {
		if ( 'R'==gprs.buff[5] )
			current.compile_month=3;
		else
			current.compile_month=5;
	} else if ( 'F'==gprs.buff[3] ) {
		current.compile_month=2;
	} else if ( 'S'==gprs.buff[3] ) {
		current.compile_month=9;
	} else if ( 'O'==gprs.buff[3] ) {
		current.compile_month=10;
	} else if ( 'N'==gprs.buff[3] ) {
		current.compile_month=11;
	} else if ( 'D'==gprs.buff[3] ) {
		current.compile_month=12;
	} else {
		/* error parsing, shouldn't happen */
		current.compile_month=255;
	}
	current.compile_year =(gprs.buff[7]-'0')*10;
	current.compile_year+=(gprs.buff[8]-'0');
	current.uptime=0;

	gprs.state=GPRS_STATE_DISCONNECTED;
	gprs.connect_state=0;
	gprs.connect_retries=0;
	gprs.connects_failed=0;

	gprs.now_generate_message=0;
	gprs.uptime=0;
	gprs.buff_length=0;
	gprs.age=65535;
	gprs.age_response=0;
	gprs.message_waiting=0;
	gprs.checksum_last_msb=0;
	gprs.checksum_last_lsb=0;
	gprs.checksum_rx_msb=0;
	gprs.checksum_rx_lsb=0;
	gprs.checksum_rx_used=1;
	gprs.connection_open=0;
	gprs.missed_acks=0;

	wireless.now_generate_message=0;
	wireless.buff_length=0;
	wireless.age=65535;
	wireless.age_response=65535;
	wireless.message_waiting=0;

	current.serial_prefix=read_eeprom(EE_SERIAL_PREFIX);
	current.serial_msb=read_eeprom(EE_SERIAL_MSB);
	current.serial_lsb=read_eeprom(EE_SERIAL_LSB);

	current.hardware_type=read_eeprom(EE_HW_TYPE);

	log.page_requested=65535;


	/* initialize other devices */
	dataflash_init();
	init_ext_fram();
	lcd_init();

	init_rtc();
	if ( bcd2bin(read_rtc(0x06)) < 5 ) {
		reset_rtc();
	}

	/* 8 MHz clock rate */
	/* limit to one periodic timer so as not to cause too much jitter */
	/* 0.1mS (100uS) period with 8 MHz clock rate */
	setup_timer_2(T2_DIV_BY_4,48,1); 

	/* receive data from serial ports */
	enable_interrupts(INT_RDA);
	enable_interrupts(INT_RDA2);
	enable_interrupts(INT_TIMER2);

	port_b_pullups(TRUE);
	delay_ms(1);
	enable_interrupts(INT_RB);
	
	ext_int_edge(0,H_TO_L);
	enable_interrupts(INT_EXT);
	enable_interrupts(GLOBAL);
}

void rdLoggerInit(void) {
//	if ( make16(current.serial_msb,current.serial_lsb) > 50 ) 
//		current.hardware_type=HARDWARE_TYPE_RDLOGGERCELL;
//	else
//		current.hardware_type=HARDWARE_TYPE_RDLOGGER;
//		current.hardware_type=HARDWARE_TYPE_RDLOGGERUNIVERSAL;

	if ( HARDWARE_TYPE_RDLOGGERCELL == current.hardware_type )  {
		/* UART2 defaults with #use_rs232 to settings for GPRS modem */
		gps_clear();
		uart_init();	
		output_high(GPRS_ON);
	} else {
		/* UART2 connects direct to mmcDaughter, so we need be at 9600 */
		set_uart_speed(9600,stream_gprs);
	}
}

void secondTasks(void) {
	static int8 last_minute=100;

	action.now_second=0;

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

		if ( GPRS_STATE_READY==gprs.state && gprs.uptime<65535 ) {
			gprs.uptime++;
		}

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
}


void serialNumberCheck(void) {
	/* 	middle button sets serial number */
	if ( 'R' != read_eeprom(EE_SERIAL_PREFIX) || read_eeprom(EE_HW_TYPE) > HARDWARE_TYPE_RDLOGGERUNIVERSAL ) {
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
		printf(lcd_putch,"rdLogger(U)");
	else if ( HARDWARE_TYPE_RDLOGGERUNIVERSAL == current.hardware_type ) 
		printf(lcd_putch,"rdLoggerUniversal");
	else
		printf(lcd_putch,"rdLoggerCell(U)");	

	serial=make16(read_eeprom(EE_SERIAL_MSB),read_eeprom(EE_SERIAL_LSB));

	for ( i=10 ; i>0 ; i-- ) {
		gprs.age_response=0;
		lcd_goto(LCD_LINE_TWO);
		printf(lcd_putch,"Up in %u ID=%c%lu ",i,read_eeprom(EE_SERIAL_PREFIX),serial);
		fprintf(stream_wireless,"# UP IN %u SECONDS\r\n",i);

		while ( gprs.age_response < 1000 )
			restart_wdt();
	}
	

	fprintf(stream_wireless,"# rdLoggerUniversal %s (20%02u-%02u-%02u) (serial %c%lu) (hardware %u)\r\n",
		__DATE__,
		current.compile_year,
		current.compile_month,
		current.compile_day,
		read_eeprom(EE_SERIAL_PREFIX),
		serial,
		current.hardware_type
	);
}

void main(void) {

	basicInit();

	wirelessOn(30);

	delay_ms(500);

	/* start the modem booting */
	lcd_clear();
	serialNumberCheck();
	rdLoggerInit();

	startupCountdown();

	/* hold down all three buttons to reset the log */
//	if ( 1==action.up_now && 1==action.down_now && 1==action.select_now ) {
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

	/* main loop */
	for ( ; ; ) {
		restart_wdt();

//		output_toggle(MMC_STATUS_TO_SD);

		if ( HARDWARE_TYPE_RDLOGGERCELL == current.hardware_type ) {
			/* GPRS related stuff */
			if ( gprs.now_generate_message ) {
				messageFromBuff();
				gprs.now_generate_message=0;
			}

			gprsTick();
		}

		/* wireless 802.15.4 related stuff */
		if ( wireless.now_generate_message ) {
			wirelessMessageFromBuff();
			wireless.now_generate_message=0;
		}

		wirelessTick();

		if ( action.now_second ) {
			secondTasks();
		}

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

		if ( HARDWARE_TYPE_RDLOGGERCELL == current.hardware_type ) {
			/* GPS */
			if ( action.now_gps_update ) {
				/* do we need to flush our FIFO to avoid getting stale data? */
				output_high(GPS_EN);
		
				if ( uart_kbhit() ) {
					gps_collect(uart_getc());
					if ( gps.valid ) {
						gps_action();
					}
				}
			} else {
				output_low(GPS_EN);
			}
		}
	}
}


#if 0
	/* left button does GPS test */
	if ( 0==input(BUTTON_0) && 1==input(BUTTON_1) && 1==input(BUTTON_2) ) {
		lcd_clear();
		lcd_putch("GPS echo to Xbee");
		lcd_goto(LCD_LINE_TWO);
		lcd_putch("SELECT TO END");

		fprintf(stream_wireless,"# about to start GPS receive test\r\n");
		for ( ; ; ) {
			output_high(GPS_EN);
			if ( uart_kbhit() )  {
				fputc(uart_getc(),stream_wireless);
			}

			if ( action.select_now ) {
				action.select_now=0;
				break;
			}
			
			restart_wdt();
			wirelessOn(120);
		}
	}
#endif
