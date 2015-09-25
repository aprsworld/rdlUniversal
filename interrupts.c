#int_timer2
void isr_100us(void) {
	static int8 ms=0;
	static int16 b0_state=0;
	static int16 b1_state=0;
	static int16 b2_state=0;

//	output_high(AN1_FILTERED);
	if ( 0xffff != timers.pulse_period ) 
		timers.pulse_period++;

	ms++;
	if ( 10 == ms ) {
		ms=0;

		/* update timers / counters */
//		if ( gprs.age < 65535 ) gprs.age++;
//		if ( gprs.age_response < 65535 ) gprs.age_response++;
		if ( wireless.age < 65535 ) wireless.age++;
		if ( wireless.age_response < 65535 ) wireless.age_response++;
	
		/* set flags if we have a message ready */
//		if ( gprs.buff_length && gprs.age > 50 && gprs.age < 65535 ) gprs.now_generate_message=1;
		if ( wireless.buff_length && wireless.age > 5 && wireless.age < 65535 ) wireless.now_generate_message=1;
	
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

//	output_low(AN1_FILTERED);
}

#int_ext
/* high resolution pulse timer / counter triggered on falling edge */
void isr_ext0(void) {
	static short state=0;
	
	output_high(WV1_FILTERED);
	current.pulse_count++;

	if ( 1 == state ) {
		/* currently counting, time to finish */
		current.pulse_period=timers.pulse_period;
		if ( current.pulse_period < current.pulse_min_period ) {
			current.pulse_min_period=current.pulse_period;
		}
		state=0;
	}

	if ( 0 == state ) {
		/* not counting, time to start */
		timers.pulse_period=0;
		state=1;
	}
	output_low(WV1_FILTERED);
}


#int_rb
void isr_rb(void) {
	int8 b;

	/* current port b must be read before interrupt will quite firing */
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
}



#int_rda
void serial_isr_wireless(void) {
	int8 c;

	wireless.age=0;
	c=fgetc(stream_wireless);

	if ( wireless.buff_length < sizeof(wireless.buff) ) {
		wireless.buff[wireless.buff_length]=c;
		wireless.buff_length++;
	}
}

#int_rda2
void serial_isr_gprs(void) {
	int8 c;

	c=fgetc(stream_gprs);
	
}
