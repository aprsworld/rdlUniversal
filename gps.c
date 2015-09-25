

void gps_clear(void) {
	gps.latitude=0.0;
	gps.longitude=0.0;
	gps.year=0;
	gps.month=0;
	gps.day=0;
	gps.hour=0;
	gps.minute=0;
	gps.second=0;
	gps.valid=0;
}

int8 xtoi(int8 h) {
	if ( h>='0' && h<='9' ) {
		return h-'0';
	}

	h=toupper(h);
	if ( h>='A' && h<='F' ) {
		return (10 + (h-'A'));
	}

	return 0;
}

#define GPS_STATE_WAITING 0
#define GPS_STATE_IN      1
#define GPS_STATE_CRC     2

void gps_collect(int8 c) {
	static int8 state=GPS_STATE_WAITING;
	static int8 crc=0;
	static int8 pos=0;

//	fputc(c,modem);

	if ( '$'==c || GPS_STATE_WAITING==state ) {
		state=GPS_STATE_IN;
		gps.valid=0;
		pos=0;
		crc=0;
	} else if ( GPS_STATE_IN==state ) {
		/* got our CRC delimiter */
		if ( '*'==c ) {
			state=GPS_STATE_CRC;
			gps.buff[pos]='\0';
			gps.buff_rcrc[0]='\0';
			pos=0;
			return;
		}

		/* add to CRC and buffer */
		crc = crc ^ c;
		if ( pos<sizeof(gps.buff) ) 
			gps.buff[pos++]=c;
	} else if ( GPS_STATE_CRC==state ) {
		if ( '\r'==c || '\n'==c ) {
			/* check CRC and set flag */
			gps.rcrc = 16*xtoi(gps.buff_rcrc[0]) + xtoi(gps.buff_rcrc[1]);

			if ( gps.rcrc == crc )
				gps.valid=1;
			state=GPS_STATE_WAITING;
			return;
		}

		if ( pos<sizeof(gps.buff_rcrc) ) 
			gps.buff_rcrc[pos++]=c;
	}

}

int8 gps_rmc_parse(void) {
	int8 buff[12];
	int8 i;
	char *p;

	/* gps.buff
	 GPRMC,235959,A,3851.3651,N,09447.9382,W,000.0,221.9,071103,003.3,E 
	 0     1      2 3         4 5          6 7     8     9      10    11
	*/

	if ( ! ('G'==gps.buff[0] && 'P'==gps.buff[1] && 'R'==gps.buff[2] && 'M'==gps.buff[3] && 'C'==gps.buff[4]) ) {
		/* not GPRMC */
		return 1;
	}

	if ( strlen(gps.buff) < 50 ) {
		/* too short of sentence */
		return 2;
	}

	/* hour */
	p=gps.buff+6;
	buff[0]=p[0];
	buff[1]=p[1];
	buff[2]='\0';
	gps.hour=atoi(buff);

	/* minute */
	p+=2;
	buff[0]=p[0];
	buff[1]=p[1];
	buff[2]='\0';
	gps.minute=atoi(buff);

	/* second */
	p+=2;
	buff[0]=p[0];
	buff[1]=p[1];
	buff[2]='\0';
	gps.second=atoi(buff);
	for ( i=0 ; i<11 && p[0]!=',' ; i++ ) p++;

	/* status */
	p+=1;
	if ( 'A'!= p[0] ) {
		gps.valid=0;
		return 3;
	}

	/* latitude */
	p+=2;
	for ( i=0 ; i<10 && p[i]!=',' ; i++ ) {
		buff[i]=p[i];
	}
	buff[++i]='\0';
	gps.latitude=atof(buff);
	
	/* latitude sign */
	p+=i;
	if ( 'S'==p[0] )
		gps.latitude=0.0-gps.latitude;

	/* longitude */
	p+=2;
	for ( i=0 ; i<11 && p[i]!=',' ; i++ ) {
		buff[i]=p[i];
	}
	buff[++i]='\0';
	gps.longitude=atof(buff);
	
	/* longitude sign */
	p+=i;
	if ( 'W'==p[0] )
		gps.longitude=0.0-gps.longitude;

	/* speed over ground, to be skipped */
	p+=2;
	for ( i=0 ; i<11 && p[0]!=',' ; i++ ) p++;

	/* course over ground, to be skipped */
	p++;
	for ( i=0 ; i<5 && p[0]!=',' ; i++ ) p++;

	/* UTC date */
	/* day */
	p++;
	buff[0]=p[0];
	buff[1]=p[1];
	buff[2]='\0';
	gps.day=atoi(buff);

	/* month */
	p+=2;
	buff[0]=p[0];
	buff[1]=p[1];
	gps.month=atoi(buff);

	/* year */
	p+=2;
	buff[0]=p[0];
	buff[1]=p[1];
	gps.year=atoi(buff);

	/* ignore the rest */

	return 0;
}


void gps_action(void) {
	if ( 0 == gps_rmc_parse() && 1 == gps.valid ) {
		output_low(GPS_EN);
		action.now_gps_update=0;

		/* set the clock */
		write_rtc(0x06,bin2bcd(gps.year));
   		write_rtc(0x05,bin2bcd(gps.month));
   		write_rtc(0x04,bin2bcd(gps.day));
		write_rtc(0x02,bin2bcd(gps.hour));
		write_rtc(0x01,bin2bcd(gps.minute));
		write_rtc(0x00,0b01111111 & bin2bcd(gps.second));

		update_time_rtc();

		/* send a status if we got a valid GPS */
		action.now_live_status=1;
	} else {
		gps_clear();
	}
}


#if 0
void gps_print(void) {
	printf("valid: %u\n",gps.valid);
	printf("lat:   %f (%d bytes)\n",gps.latitude,sizeof(gps.latitude));
	printf("lon:   %f (%d bytes)\n",gps.longitude,sizeof(gps.longitude));
	printf("date:  20%02u-%02u-%02u\n",gps.year,gps.month,gps.day);
	printf("time:  %02u:%02u:%02u\n",gps.hour,gps.minute,gps.second);
	printf("rcrc:  0x%02x\n",gps.rcrc);
}


int main(int argc, char **argv) {
	int8 i;
	char sample[512];

	strcpy(sample,"$GPRMC,235959,A,3851.3651,N,09447.9382,W,000.0,221.9,071103,003.3,E*69\r\n");
	strcat(sample,"$GPRMC,222002,A,3614.4133,N,11509.6897,W,0.0,355.3,070401,13.1,E,A*31\r\n");
	strcat(sample,"$GPRMC,222128.00,A,4211.1283,N,08805.4656,W,0.0,341.5,160301,,*24\r\n");
	

	printf("# parsing: %s\n",sample);
	gps_clear();

	for ( i=0 ; i<strlen(sample) ; i++ ) {
		gps_collect(sample[i]);
		if ( gps.valid ) {
			printf("# have valid NMEA sentence (%s):\n",gps.buff);
			gps_parse();
			gps_print();
			gps_clear();
		}
	}

	return 0;
}
#endif
