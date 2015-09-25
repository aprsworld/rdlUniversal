#include <stdio.h>
#include <string.h>
#include "gps.h"

typedef unsigned char int8;
typedef short int16;

GPS_FIX fixdata;

int main(int argc, char **argv) {
	int8 i;
	char sample[128];

	nmea_init();

	strcpy(sample,"$GPRMC,235959,A,3851.3651,N,09447.9382,W,000.0,221.9,071103,003.3,E*69");

	printf("# parsing: %s\n",sample);

	for ( i=0 ; i<strlen(sample) ; i++ ) {
		gps_parse(sample[i]);
	}



	return 0;
}
