//#define	lcd_cursor(x)	lcd_write(((x)&0x7F)|0x80)
#define 	LCD_LINE_ONE			0x00	/* position of line1 */
#define 	LCD_LINE_TWO			0x40	/* position of line2 */

#define		LCD_CURSOR_BLINK		0b1111
#define		LCD_CURSOR_ON			0b1110
#define		LCD_CURSOR_OFF			0b1100

#define DEG_SYMBOL 0xDF

void LCD_STROBE(void) {
	output_high(LCD_EN);
//	delay_ms(5);
	output_low(LCD_EN);
}

/* write a byte to the LCD in 4 bit mode */
void lcd_write(unsigned char c) {
	output_bit(LCD_D7,c & 0x80);
	output_bit(LCD_D6,c & 0x40);
	output_bit(LCD_D5,c & 0x20);
	output_bit(LCD_D4,c & 0x10);
	LCD_STROBE();
	output_bit(LCD_D7,c & 0x08);
	output_bit(LCD_D6,c & 0x04);
	output_bit(LCD_D5,c & 0x02);
	output_bit(LCD_D4,c & 0x01);
	LCD_STROBE();	
	delay_us(40);
}

/* 	Clear and home the LCD */
void lcd_clear(void) {
	output_low(LCD_RS);

	lcd_write(0x1);
	delay_ms(2);
}

void lcd_cursor(int command) {
	output_low(LCD_RS);
	lcd_write(command);
	
	delay_us(40);
}




/* write one character to the LCD */
void lcd_putch(unsigned char c) {
	output_high(LCD_RS);

	lcd_write(c);
}


/* Go to the specified position */
void lcd_goto(unsigned char lpos) {
	output_low(LCD_RS);

	lcd_write(0x80 + lpos);
}

	
/* initialise the LCD - put into 4 bit mode */
void lcd_init(void) {

	/* step 1 */
	delay_ms(15);// power on delay
	
	/* step 2 */
	output_low(LCD_RS);
	output_low(LCD_RW);
	output_low(LCD_D7);
	output_low(LCD_D6);
	output_high(LCD_D5);
	output_high(LCD_D4);
	
	/* step 3 */
	LCD_STROBE();	
	delay_ms(5);

	/* step 4 and 5 */
	LCD_STROBE();	// init!	
	delay_us(100);

	/* step 6 */
	LCD_STROBE();	// init!	
	delay_ms(5);

	output_low(LCD_D4);
	LCD_STROBE();	
	delay_us(40);
	
	lcd_write(0x28);// 4 bit mode, 1/16 duty, 5x8 font, 2lines
	lcd_write(0x0C);// display on
	lcd_write(0x06);// entry mode advance cursor
	lcd_write(0x01);// clear display and reset cursor
}
