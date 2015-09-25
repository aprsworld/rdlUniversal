#define MEDIAN_FILTER_WIDTH  7
#define MEAN_FILTER_WIDTH    16

//------------------------------------------
// FUNCTION PROTOTYPES

int16 median_filter(int16 latest_element);
void Insertion_Sort_16(int16 *data, char array_size);
int16 mean_filter(int16 latest_element);


// filters.c
//-------------------
// median_filter:
// This function sorts an array of longs so it's in
// ascending order. It then returns the middle element.
// ie., If the array has 7 elements (0-6), it returns
// the element at index 3.  The user should ensure that
// the array has an odd number of elements.
// This function stores data from prior calls in a static
// buffer.

// The output of this function will always be N/2 +1
// elements behind the input data, where N is the filter width.

int16 median_filter(int16 latest_element) {
	static int16 input_buffer[MEDIAN_FILTER_WIDTH];
	static char inbuf_index = 0;
	static char num_elements = 0;
	int16 sorted_data[MEDIAN_FILTER_WIDTH];
	int16 median;

	// Insert incoming data element into circular input buffer.
	input_buffer[inbuf_index] = latest_element;
	inbuf_index++;
	if(inbuf_index >= MEDIAN_FILTER_WIDTH)  // If index went past buffer end
   	inbuf_index = 0;       // then reset it to start of buffer

	if(num_elements < MEDIAN_FILTER_WIDTH)
	   num_elements++;

	// THIS LINE MAY NOT BE NEEDED IF SORTED DATA IS STATIC.
	memset(sorted_data, 0, MEDIAN_FILTER_WIDTH * 2);

	// Copy input data buffer to the (to be) sorted data array.
	memcpy(sorted_data, input_buffer, num_elements * 2);   // memcpy works on bytes

	// Then sort the data.
	Insertion_Sort_16(sorted_data, MEDIAN_FILTER_WIDTH);

	// During the first few calls to this function, we have fewer
	// elements in the sorted data array than the filter width.
	// So to compensate for that, we pick the median from the number
	// of elements that are available.  ie, if we have 3 elements,
	// we pick the middle one of those as the median.
	// Also, because the sort function sorts the data from low to high,
	// we have to calculate the index from the high end of the array.
	median = sorted_data[MEDIAN_FILTER_WIDTH - 1 - num_elements/2];

	return(median);
}

//-------------------------------------------------------------
// This function calculates the Mean (average).

int16 mean_filter(int16 latest_element) {
	static int16 input_buffer[MEAN_FILTER_WIDTH];
	static char inbuf_index = 0;
	static char num_elements = 0;
	int32 mean;
	int32 sum;
	char i;

	// Insert incoming data element into circular input buffer.
	input_buffer[inbuf_index] = latest_element;
	inbuf_index++;
	if(inbuf_index >= MEAN_FILTER_WIDTH)  // If index went past buffer end
	   inbuf_index = 0;       // then reset it to start of buffer

	if(num_elements < MEAN_FILTER_WIDTH)
	   num_elements++;

	// Calculate the mean.  This is done by summing up the
	// values and dividing by the number of elements.
	sum = 0;
	for(i = 0; i < num_elements; i++)
		sum += input_buffer[i];

	// Round-off the result by adding half the divisor to
	// the numerator.
	mean = (sum + (int32)(num_elements >> 1)) / num_elements;

	return((int16)mean);
}

//-----------------------------------------------------
void Insertion_Sort_16(int16 *data, char array_size) {
	char i, j;
	int16 index;

	for(i = 1; i < array_size; i++) {
		index = data[i];
		j = i;
		
		while ((j > 0) && (data[j-1] > index)) {
			data[j] = data[j-1];
			j = j - 1;
		}
		
		data[j] = index;
	}
}

/* 
convert wind direction to a 45 degree sector with 0 at North, 4 at South
see windDirection_sector.xls spreadsheet
*/
int8 wind_sector(int16 a) {
	if ( a>=64 && a<192 )
		return 1;
	if ( a>=192 && a<320 )
		return 2;
	if ( a>=320 && a<448 )
		return 3;
	if ( a>=448 && a<576 )
		return 4;
	if ( a>=576 && a<704 )
		return 5;
	if ( a>=704 && a<832 )
		return 6;
	if ( a>=832 && a<960 )
		return 7;

	return 0;
}

int8 battery_charge(int16 a) {
	if ( a >= 325 ) return 10;
	if ( a >= 320 ) return 9;
	if ( a >= 318 ) return 8;
	if ( a >= 315 ) return 7;
	if ( a >= 312 ) return 6;
	if ( a >= 308 ) return 5;
	if ( a >= 304 ) return 4;
	if ( a >= 301 ) return 3;
	if ( a >= 296 ) return 2;
	if ( a >= 289 ) return 1;
	return 0;
}

void sample_adc(void) {
	int j;
	int16 value;

	/* battery voltage */
	set_adc_channel(ADC_AN_IN_VOLTS);	
	delay_ms(1);

	/* sample 16 times and mean filter */
	for ( j=0 ; j<16 ; j++ ) {
		value = mean_filter(read_adc());
	}
	current.input_voltage_adc=value;
	current.battery_charge=battery_charge(value);

	/* wind direction */
	if ( HARDWARE_TYPE_RDLOGGERUNIVERSAL == current.hardware_type )  {
		set_adc_channel(ADC_WV0_FILTERED);
	} else {
		set_adc_channel(3);
	}
	delay_ms(1);
	current.wind_direction_sector=wind_sector(read_adc());
}



