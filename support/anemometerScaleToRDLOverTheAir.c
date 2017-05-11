#include <stdio.h>

#define INPUT_ANEMOMETER_M 0.04598
#define INPUT_ANEMOMETER_B 0.24786

#define OUTPUT_ANEMOMETER_M 0.765
#define OUTPUT_ANEMOMETER_B 0.35

//#define STOP_WIND_SPEED 50.0
#define STOP_HZ 1200.0

int main(int argc, char **argv) {
	int inHz;
	double windSpeed;
	double outHz;
	double outOTA;

	printf("#define HZ_TO_40HC_OTA_MAX %0.0f\n",STOP_HZ);

	printf("const int16 hz_to_40hc_OTA[] = {\n");


	for ( inHz=0.0 ; inHz<=STOP_HZ ; inHz += 1.0 ) {
//		inHz = ( windSpeed - INPUT_ANEMOMETER_B ) / INPUT_ANEMOMETER_M;
		windSpeed = INPUT_ANEMOMETER_M * inHz + INPUT_ANEMOMETER_B;
		outHz = ( windSpeed - OUTPUT_ANEMOMETER_B ) / OUTPUT_ANEMOMETER_M;
		outOTA = 10000.0 / outHz;

		if ( outOTA < 0 || outOTA > 65534 ) {
			outOTA=0.0;
		}


		printf("\t%5.0f%c /* inHz=%4d, windSpeed=%2.1f, outHz=%4.1f */\n",
			outOTA,
			inHz != STOP_HZ ? ',' : ' ',
			inHz,
			windSpeed,
			outHz
		);

	}

	printf("};\n");

	return 0;
}