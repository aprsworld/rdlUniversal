#int_timer3
/* this timer only runs during boot-up to do key presses */
void isr_10ms(void) {
	static int16 b0_state=0;
	static int16 b1_state=0;
	static int16 b2_state=0;

	set_timer3(45536);

	action.port_b=port_b;

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

#int_timer2 HIGH
/* this timer only runs once booted */
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
