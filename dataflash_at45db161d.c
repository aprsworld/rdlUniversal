#define DATAFLASH_PAGES 4096
#define DATAFLASH_BYTES_PER_PAGE 528


/*
#define DATAFLASH_CS       PIN_D2
#define DATAFLASH_RESET    PIN_D3
#define DATAFLASH_SI       PIN_D4
#define DATAFLASH_SO       PIN_D5
#define DATAFLASH_SCK      PIN_D6
*/

#inline
#define dataflash_select()\
        output_low(DATAFLASH_CS);\
        delay_cycles(1)

#inline
#define dataflash_unselect()\
        output_high(DATAFLASH_CS);\
        delay_cycles(1)   


int8 dataflash_read_status(void) {
    int8 status;

	dataflash_select();

	/* "status register read" */
	spi_write2(0xd7);

	status=spi_read2(0);

	dataflash_unselect();

	return status;
}

void dataflash_init() {
	setup_spi2(SPI_MASTER|SPI_H_TO_L|SPI_SAMPLE_AT_END);//SPI_CLK_DIV_4
	output_high(DATAFLASH_RESET);
	dataflash_unselect();

	
//	fprintf(modem,"dataflash read status: 0x%02X\r\n",dataflash_read_status());
}
 
void dataflash_deep_power_down(void) {
	dataflash_select();
	spi_write2(0xB9);
	dataflash_unselect();
}

/* resume from deep power-down */
void dataflash_resume(void) {
	dataflash_select();
	spi_write2(0xAB);
	dataflash_unselect();
	delay_ms(1);
}

/* read a whole page of data starting at page boundary (byte 0) */
void dataflash_page_read(int16 page) {
	int16 l;

	dataflash_select();
	spi_write2(0xD2);
	page = page << 2;
	spi_write2(make8(page,1));
	spi_write2(make8(page,0));
	spi_write2(0x00); 

	/* four don't care bytes */
	spi_write2(0x00);
	spi_write2(0x00);
	spi_write2(0x00);
	spi_write2(0x00);

	fprintf(stream_wireless,"# Dataflash page read from %lu\r\n",page>>2);
	/* start reading */
	for ( l=0 ; l<DATAFLASH_BYTES_PER_PAGE ; l++ ) {
		fprintf(stream_wireless,"%02X ",spi_read2(0x00));

		if ( 0 != l && 0 == l%16 ) {
			fprintf(stream_wireless,"\r\n");
		}
	}
	fprintf(stream_wireless,"\r\n# Done\r\n");
	dataflash_unselect();

}

void dataflash_page_write(int16 page) {
	int16 l;

	/* write to buffer 1 */
	dataflash_select();
	spi_write2(0x82);
	/* start at byte 0 */
	spi_write2(0x00);
	spi_write2(0x00);
	spi_write2(0x00);
	for ( l=0 ; l<DATAFLASH_BYTES_PER_PAGE ; l++ ) {
		spi_write2(make8(l,0));
	}
	dataflash_unselect();

	/* wait for page to finish erasing and writing */
	while ( ! bit_test(dataflash_read_status(),7) ) {
		//fputc('x',modem);
	}
}