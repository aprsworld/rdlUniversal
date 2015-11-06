#int_timer2 HIGH
void isr_100us(void) {
	static int8 tick=0;

	/* anemometer polling state variables */
	/* anemometer 0 / PIN_B0 */
	short ext0_count;
	short ext0_now;
	static short ext0_last=0;
	static short ext0_state=0;

	/* count time between falling edges */
	if ( ext0_count && 0xffff != timers.pulse_period )
		timers.pulse_period++;

	/* anemometer 0 / PIN_B0 trigger on falling edge */
	ext0_now=input(PIN_B0);
	if ( 0 == ext0_now && 1 == ext0_last ) {
		current.pulse_count++;
		if ( 1 == ext0_state ) {
			/* currently counting, time to finish */
			ext0_count=0;
			current.pulse_period=timers.pulse_period;
			if ( current.pulse_period < current.pulse_min_period ) {
				current.pulse_min_period=current.pulse_period;
			}
			ext0_state=0;
		}
		if ( 0 == ext0_state ) {
			/* not counting, time to start */
			timers.pulse_period=0;
			ext0_count=1;
			ext0_state=1;
		}
	}
	ext0_last = ext0_now;

	/* every 100 cycles we tell main() loop to do 10 milisecond activities */
	tick++;
	if ( 100 == tick ) {
		tick=0;
		action.now_10millisecond=1;
	}
}



#if 0
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
#endif



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
void serial_isr_sd(void) {
	int8 c;

	c=fgetc(stream_sd);
}
