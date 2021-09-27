/*
	author: Matthew Boeding
	version: v3.0
  Adapted from: Subharthi Banerjee, Ph.D.

	README

/// The sole reason to provide this code is to make your TFTLCD (ILI9341)
/// up and running

/// Note: Most of the code is in one place. This is not ideal and I plan to change
///   it in the future

/// Use C or inline assembly program as you please.

/// ** the code uses P0 for 8-bit interface
/// ** IOM --> P3^4
/// ** CD --> P3^5
////  I recommend leaving these definitions for UART implementation later.
////
/// ** RD --> P3^3
/// ** WR --> P3^2

/// Refer to the header file to change decoding addresses for your specific design.

/// Please do not post any of the code from this course to GITHUB.

*/


///  *************** IMPORTANT *********************

// It may need redfinition of pins like
// #include <reg51.h> // change microcontroller header when using AT89C55WD









# include "ecen4330lcdh.h"
# include "font.h"

// keypad configuration
__code unsigned char keypad[4][4] =	{{'1','4','7','E'},
				{'2','5','8','0'},
				{'3','6','9','F'},
				{'A','B','C','D'} };
unsigned char colloc, rowloc;
// store it in a variable the lcd address
__xdata unsigned char* lcd_address = (unsigned char __xdata*) __LCD_ADDRESS__;
__xdata unsigned char* seg7_address = (unsigned char __xdata*) __SEG_7_ADDRESS__;
__xdata unsigned char* read_ram_address;


 # define write8inline(d) {			\
 	IOM = 1;							\
	*lcd_address = d; 						\
	IOM = 0;							\
}


# define write8 write8inline
// data write
# define write8DataInline(d) {	\
	CD = 1; 					\
	write8(d);					\
}
// command or register write
# define write8RegInline(d) {	\
	CD = 0; 					\
	write8(d);					\
}

// inline definitions
# define write8Reg write8RegInline
# define write8Data write8DataInline




u16 cursor_x, cursor_y;  /// cursor_y and cursor_x globals
u8 textsize, rotation; /// textsize and rotation
u16
    textcolor,      ////< 16-bit background color for print()
    textbgcolor;    ////< 16-bit text color for print()
u16
    _width,         ////< Display width as modified by current rotation
    _height;        ////< Display height as modified by current rotation

volatile unsigned char received_byte=0;
volatile unsigned char recieved_flag = 0;

void ISR_receive() __interrupt (4) {
	if (RI == 1){
		received_byte = SBUF - 0x40;
		RI = 0;
		recieved_flag= 1;

	}
}


void UART_Init(){
    SCON = 0x50;  // Asynchronous mode, 8-bit data and 1-stop bit
    TMOD = 0x20;  // Timer1 in Mode2. in 8 bit auto reload
    TH1 =  0xFD;  // Load timer value for 9600 baudrate
    TR1 = 1;      // Turn ON the timer for Baud rate generation
    ES  = 1;      // Enable Serial Interrupt
    EA  = 1;      // Enable Global Interrupt bit
}


void UART_transmit(unsigned char byte){
    SBUF = byte;
    while(TI == 1);
    TI = 0;
}
inline void iowrite8(unsigned char __xdata* map_address, unsigned char d) {
	IOM = 1;
	*map_address = d;
	IOM = 0;
}

inline unsigned char ioread8(unsigned char __xdata* map_address) {
	unsigned char d = 0;
	IOM = 1;							
	d = *map_address;
	IOM = 0;	
	return d;	
}

void delay (int d) /// x 1ms
{
	int i,j;
	for (i=0;i<d;i++) /// this is For(); loop delay used to define delay value in Embedded C
	{
	for (j=0;j<1000;j++);
	}
}




void writeRegister8(u8 a, u8 d) {
	//IOM = 0;
	CD = __CMD__;
	write8(a);
	CD = __DATA__;
	write8(d);
	//IOM = 1;
}



void writeRegister16(u16 a, u16 d){
	unsigned short int hi, lo;
 	hi = (a) >> 8;
 	lo = (a);
	//IOM = 0;
 //	CD = 0;
 	write8Reg(hi);
 	write8Reg(lo);
  	hi = (d) >> 8;
  	lo = (d);
  	CD = 1 ;
  	write8Data(hi);
  	write8Data(lo);
//	IOM =1;
}


/// +=======================RTC functions===============================
void rtcInit(void) {
	//rtcCmd(__REG_F__, __HR_24__);
	unsigned int i;
	rtcCmd(__REG_F__, __HR_24__|__STOP__|__RESET__);  // stop and reset
	
	// clear the registers
	for (i = __S1_REG__; i < __REG_D__;i++) {
		rtcWrite(i, 0x00);
	}
	
	rtcCmd(__REG_F__, __HR_24__);

}

void rtcBusy(void) {
	__xdata unsigned char* map_address =  (unsigned char __xdata*) (__REG_D__);
	while((ioread8(map_address) & 0x02));
}

inline void rtcCmd(unsigned int addr, unsigned char d) {
	__xdata unsigned char* map_address =  (unsigned char __xdata*) addr;
	iowrite8(map_address, d);
}

inline void rtcWrite(unsigned int addr, unsigned char d) {
	__xdata unsigned char* map_address =  (unsigned char __xdata*) addr;
	rtcCmd(__REG_D__, 0x01);
	rtcBusy();
	iowrite8(map_address, 0x00);
	rtcCmd(__REG_D__, d);
}

inline unsigned char rtcRead(unsigned int addr) {
	unsigned char d;
	__xdata unsigned char* map_address =  (unsigned char __xdata*) addr;
	rtcCmd(__REG_D__, 0x01); // hold on
	rtcBusy();
	d = ioread8(map_address);

	d = (d & 0x0f) | 0x30; // ascii the lower word
	rtcCmd(__REG_D__, 0x00); // hold off
	return d;
}

void rtcPrint(void) {
	unsigned char mi1, mi10, s1, s10, h1, h10;
	unsigned char printval[9];
	printval[8] = '\0'; // end with a null character for string
	printval[2] = ':';
	printval[5] = ':';
 
	mi1 = 0x30; mi10=0x30;s1=0x30;s10=0x30;h1=0x30;h10=0x30; // char zero
	mi1 = rtcRead(__MI1_REG__);
	mi10 = rtcRead(__MI10_REG__);
	h1 = rtcRead(__H1_REG__);
	h10 = rtcRead(__H10_REG__);
	s1 = rtcRead(__S1_REG__);
	s10 = rtcRead(__S10_REG__);	
	printval[0] = h10;
	printval[1] = h1;
	printval[3] = mi10;
	printval[4] = mi1;
	printval[6] = s10;
	printval[7] = s1;
	setCursor(132, 304);
	LCD_string_write(printval);
}

void setCursor(u16 x, u16 y){
	cursor_x = x;
	cursor_y = y;
}
// set text color
void setTextColor(u16 x, u16 y){
	textcolor =  x;
	textbgcolor = y;
}

// set text size
void setTextSize(u8 s){
	if (s > 8) return;
	textsize = (s>0) ? s : 1 ;
}

void setRotation(u8 flag){
	switch(flag) {
		case 0:
			flag = (ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR);
			_width = TFTWIDTH;
			_height = TFTHEIGHT;
			break;
		case 1:
			flag = (ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR);
			_width = TFTHEIGHT;
			_height = TFTWIDTH;
			break;
		case 2:
			flag = (ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR);
			_width = TFTWIDTH;
			_height = TFTHEIGHT;
			break;
	  case 3:
			flag = (ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR);
			_width = TFTHEIGHT;
			_height = TFTWIDTH;
			break;
		default:
			flag = (ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR);
			_width = TFTWIDTH;
			_height = TFTHEIGHT;
			break;
	}
	writeRegister8(ILI9341_MEMCONTROL, flag);
}

// set address definition
void setAddress(unsigned int x1,unsigned int y1,unsigned int x2,unsigned int y2){
	//IOM =0;
	write8Reg(0x2A);
	write8Data(x1 >> 8);
	write8Data(x1);
	write8Data(x2 >> 8);
	write8Data(x2);

	write8Reg(0x2B);
	write8Data(y1 >> 8);
	write8Data(y1);
	write8Data(y2 >> 8);
	write8Data(y2);
	//write8Reg(0x2C);
  //IOM =1;


}

void TFT_LCD_INIT(void){
	//char ID[5];
	///int id;
	_width = TFTWIDTH;
	_height = TFTHEIGHT;

	// all low
	IOM = 0;
	//RDN = 1;
	CD = 1;

	write8Reg(0x00);
	write8Data(0x00);write8Data(0x00);write8Data(0x00);
	//IOM = 1;
	delay(200);

	//IOM = 0;

	writeRegister8(ILI9341_SOFTRESET, 0);
    delay(50);
    writeRegister8(ILI9341_DISPLAYOFF, 0);
    delay(10);






    writeRegister8(ILI9341_POWERCONTROL1, 0x23);
    writeRegister8(ILI9341_POWERCONTROL2, 0x11);
    write8Reg(ILI9341_VCOMCONTROL1);
		write8Data(0x3d);
		write8Data(0x30);
    writeRegister8(ILI9341_VCOMCONTROL2, 0xaa);
    writeRegister8(ILI9341_MEMCONTROL, ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR);
    write8Reg(ILI9341_PIXELFORMAT);
	write8Data(0x55);write8Data(0x00);
    writeRegister16(ILI9341_FRAMECONTROL, 0x001B);

    writeRegister8(ILI9341_ENTRYMODE, 0x07);
    /* writeRegister32(ILI9341_DISPLAYFUNC, 0x0A822700);*/



    writeRegister8(ILI9341_SLEEPOUT, 0);
    delay(150);
    writeRegister8(ILI9341_DISPLAYON, 0);
    delay(500);
		setAddress(0,0,_width-1,_height-1);
     ///************* Start Initial Sequence ILI9341 controller **********///

	 // IOM = 1;
}
void drawPixel(u16 x3,u16 y3,u16 color1)
{

	// not using to speed up
	//if ((x3 < 0) ||(x3 >= TFTWIDTH) || (y3 < 0) || (y3 >= TFTHEIGHT))
	//{
	//	return;
	//}
	setAddress(x3,y3,x3+1,y3+1);

	//IOM = 0;

    CD=0; write8(0x2C);

	CD = 1;
	write8(color1>>8);write8(color1);
	//IOM = 1;
}

// draw a circle with this function

void drawCircle(int x0, int y0, int r, u16 color){
	int f = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x = 0;
    int y = r;


    drawPixel(x0  , y0+r, color);
    drawPixel(x0  , y0-r, color);
    drawPixel(x0+r, y0  , color);
    drawPixel(x0-r, y0  , color);

    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        drawPixel(x0 + x, y0 + y, color);
        drawPixel(x0 - x, y0 + y, color);
        drawPixel(x0 + x, y0 - y, color);
        drawPixel(x0 - x, y0 - y, color);
        drawPixel(x0 + y, y0 + x, color);
        drawPixel(x0 - y, y0 + x, color);
        drawPixel(x0 + y, y0 - x, color);
        drawPixel(x0 - y, y0 - x, color);
    }
}


void fillRect(u16 x,u16 y,u16 w,u16 h,u16 color){
	if ((x >= TFTWIDTH) || (y >= TFTHEIGHT))
	{
		return;
	}

	if ((x+w-1) >= TFTWIDTH)
	{
		w = TFTWIDTH-x;
	}

	if ((y+h-1) >= TFTHEIGHT)
	{
		h = TFTHEIGHT-y;
	}

	setAddress(x, y, x+w-1, y+h-1);
    //IOM = 0;


	write8Reg(0x2C);
	//IOM = 1; IOM = 0;
	CD = 1;
	for(y=h; y>0; y--)
	{
		for(x=w; x>0; x--)
		{

			write8(color>>8); write8(color);

		}
	}
	//IOM = 1;
}

void fillScreen(unsigned int Color){
	//unsigned char VH,VL;
	long len = (long)TFTWIDTH * (long)TFTHEIGHT;

	 int blocks;

   unsigned char  i, hi = Color >> 8,
              lo = Color;

    blocks = (u16)(len / 64); // 64 pixels/block
	setAddress(0,0,TFTWIDTH-1,TFTHEIGHT-1);

	//IOM = 0;


	write8Reg(0x2C);
	//IOM = 1; IOM = 0;
		CD = 1;
		write8(hi); write8(lo);

		len--;
		while(blocks--) {
      i = 16; // 64 pixels/block / 4 pixels/pass
      do {

				write8(hi); write8(lo);write8(hi); write8(lo);
				write8(hi); write8(lo);write8(hi); write8(lo);
      } while(--i);
    }
    for(i = (char)len & 63; i--; ) {

      write8(hi); write8(lo);

    }

	//IOM = 1;

}
void drawChar(int x, int y, unsigned char c,u16 color, u16 bg, u8 size){
	if ((x >=TFTWIDTH) || // Clip right
	    (y >=TFTHEIGHT)           || // Clip bottom
	    ((x + 6 * size - 1) < 0) || // Clip left
	    ((y + 8 * size - 1) < 0))   // Clip top
	{
		return;
	}

	for (char i=0; i<6; i++ )
	{
		u8 line;

		if (i == 5)
		{
			line = 0x0;
		}
		else
		{
			line = pgm_read_byte(font+(c*5)+i);
		}

		for (char j = 0; j<8; j++)
		{
			if (line & 0x1)
			{
				if (size == 1) // default size
				{
					drawPixel(x+i, y+j, color);
				}
				else {  // big size
					fillRect(x+(i*size), y+(j*size), size, size, color);
				}
			} else if (bg != color)
			{
				if (size == 1) // default size
				{
					drawPixel(x+i, y+j, bg);
				}
				else
				{  // big size
					fillRect(x+i*size, y+j*size, size, size, bg);
				}
			}

			line >>= 1;
		}
	}

}

void write(u8 c)//write a character at setted coordinates after setting location and colour
{
	if (c == '\n')
	{
		cursor_y += textsize*8;
		cursor_x  = 0;
	}
	else if (c == '\r')
	{
		// skip em
	}
	else
	{
		drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize);
		cursor_x += textsize*6;
	}
}
void LCD_string_write(char *str)
{
	int i;
	for(i=0;str[i]!=0;i++)	/* Send each char of string till the NULL */
	{
		write(str[i]);	/* Call transmit data function */
	}
}

// test RAM function


void testRAM(unsigned char d){
	unsigned int i;
	
	__xdata unsigned char* ram_address;
	__code unsigned char* errstr = "Error\n";

	for (i = __START_RAM__; i<__END_RAM__; i++) {
		IOM = 0;
		ram_address = (unsigned char __xdata*)(i);
		*ram_address = d;
		IOM = 1;
	}
	

}



unsigned char keyDetect(){
	recieved_flag = 0;
	__xdata u16 x = cursor_x;
	__xdata u16 y = cursor_y;
	__KEYPAD_PORT__=0xF0;			/*set port direction as input-output*/
	do
	{
		__KEYPAD_PORT__ = 0xF0;
		colloc = __KEYPAD_PORT__;
		colloc&= 0xF0;	/* mask port for column read only */
	}while(colloc != 0xF0);		/* read status of column */

	do
	{
		do
		{
			
			if (recieved_flag == 1){
		// do something with received_byte
			recieved_flag = 0;
			return received_byte;
	}
			rtcPrint();
			setCursor(x, y);
			
			delay(20);	/* 20ms key debounce time */
			colloc = (__KEYPAD_PORT__ & 0xF0);	/* read status of column */
		}while(colloc == 0xF0);	/* check for any key press */
		
		delay(1);
		colloc = (__KEYPAD_PORT__ & 0xF0);
	}while(colloc == 0xF0);

	while(1)
	{
		/* now check for rows */
		__KEYPAD_PORT__= 0xFE;											/* check for pressed key in 1st row */
		colloc = (__KEYPAD_PORT__ & 0xF0);
		if(colloc != 0xF0)
		{
			rowloc = 0;
			break;
		}

		__KEYPAD_PORT__ = 0xFD;									/* check for pressed key in 2nd row */
		colloc = (__KEYPAD_PORT__ & 0xF0);
		if(colloc != 0xF0)
		{
			rowloc = 1;
			break;
		}

	__KEYPAD_PORT__ = 0xFB;			/* check for pressed key in 3rd row */
	colloc = (__KEYPAD_PORT__ & 0xF0);
	if(colloc != 0xF0)
	{
		rowloc = 2;
		break;
	}

	__KEYPAD_PORT__ = 0xF7;			/* check for pressed key in 4th row */
	colloc = (__KEYPAD_PORT__ & 0xF0);
	if(colloc != 0xF0)
	{
		rowloc = 3;
		break;
	}
}

	if(colloc == 0xE0)
	{
		return(keypad[rowloc][0]);
	}
	else if(colloc == 0xD0)
	{
		return(keypad[rowloc][1]);
	}
	else if(colloc == 0xB0)
	{
		return(keypad[rowloc][2]);
	}
	else
	{
		return(keypad[rowloc][3]);
	}
}

unsigned int reverse(unsigned char d) {
	unsigned int rev = 0;
	unsigned int val = 0;
	while(d >= 1){

		val = d%10;
		d = d/10;
		rev = rev * 10 + val;
	}
	return rev;
}

void asciiToDec(unsigned char d) {
	__xdata unsigned char val;
	__xdata unsigned int id;
	id = reverse(d);
	while (id >= 1){

		val = id % 10;
		id = id/10;
		write(val + '0');
	}
	write('\n');
}

unsigned char ascii2Dec(unsigned char d) {
	__xdata unsigned char val = 0;
	__xdata unsigned int id;
	id = reverse(d);
	while (id >= 1){

		val = id % 10;
		id = id/10;
		val + '0';
	}
	return val;
}

void ascii2Hex(unsigned char d) {
	__xdata unsigned char vals[2];
	vals[0] = 0;
	vals[1] = 0;
	if( d == 0)
	{
		LCD_string_write("00");
		return;
	}
	for(__xdata int i = 1; i >=0; i--)
	{
		if(d != 0)
		{
			vals[i] = d % 16;
			d /= 16;
		}
		
	}
	for(__xdata int j = 0; j <=1; j++)
	{
		
		asciiToHex1(vals[j]);
	}
	
}
void ascii4Hex(unsigned int d) {
	__xdata unsigned char vals[4];
	vals[0] = 0;
	vals[1] = 0;
	vals[2] = 0;
	vals[3] = 0;
	if( d == 0)
	{
		LCD_string_write("0000");
		return;
	}
	for(__xdata int i = 3; i >=0; i--)
	{
		if(d != 0)
		{
			vals[i] = d % 16;
			d /= 16;
		}
		
	}
	for(__xdata int j = 0; j <=3; j++)
	{
		
		asciiToHex1(vals[j]);
	}
	
}
void asciiToHex1(unsigned char d) {
	__xdata unsigned char val;
	__xdata unsigned char store[2];
	__xdata unsigned char i =0;
	store[0] = 0;
	store[1] = 0;

		val = d % 16;
		d = d/16;
		if (val <= 9) {

			store[0] = val + '0';
		}
		else {
			store[0] = (val%10) + 'A';
		}
	
	
	LCD_string_write(store);
}




void menuDisplay() {
	__code unsigned char* initString = "Grayson S.\nECEN 4330\nD-Dump B-Move\nE-Edit F-Find\nC-Count A-RCheck\n";
	LCD_string_write(initString);
	

}

void menuOperate() {
	__code unsigned char* newline = "\n";
	__code unsigned char* addline = "Address?\n";
	while (1) {
		
		LCD_CLEAR();
		
		menuDisplay();
		
		
		LCD_string_write(newline);
		unsigned char firstkey = keyDetect();
	
		write(firstkey);
		if(firstkey == 'D') {
		Dump();
		}
		if(firstkey == 'A') {
			LCD_CLEAR();
			
		ramCheck();
		}
		else if(firstkey == 'B') {
		Move();
		}
		else if(firstkey == 'E') {
			__xdata unsigned char* ram_address;
			LCD_CLEAR();
			LCD_string_write(addline);
			__xdata unsigned int address = getKeyNum(4);
			ram_address = (__xdata unsigned char*) address;
			
			Edit(address, ram_address);
		}
		else if(firstkey == 'F') {
		Find();
		}
		else if(firstkey == 'C') {
		Count();
		}
	}


}
void ramCheck() {
	__code unsigned char* str2 = "Check Complete\n";
	unsigned int i;
	__code unsigned char* err = "Error";
	__code unsigned char* str = "Value?\n";
	LCD_string_write(str);
	LCD_string_write("\n");
	unsigned char checkval = getKeyNum(2);
	unsigned char* ram_address;
	unsigned char ramval;
	unsigned char count = 0;
	
	testRAM(checkval);
	for (i = 0; i< 0xFFFF; i++) {
		IOM = 0;
		ram_address = (unsigned char __xdata*)(i);
		if(*ram_address != checkval)
		{
			ramval = *ram_address;
			LCD_string_write("\n");

			ascii4Hex(i);
			LCD_string_write("\n");
			ascii4Hex(ramval);
			LCD_string_write("\n");

			ascii4Hex(checkval);
			LCD_string_write("\n");

			LCD_string_write(err);
			delay(5);

		}
		IOM = 1;
	}
	checkval = ~checkval - 0xFF00;
	testRAM(checkval);
	for (i = 0; i< 0xFFFF; i++) {
		IOM = 0;
		ram_address = (unsigned char __xdata*)(i);
		if(*ram_address != checkval)
		{
			ramval = *ram_address;
			LCD_string_write("\n");

			ascii4Hex(i);
			LCD_string_write("\n");
			ascii4Hex(ramval);
			LCD_string_write("\n");

			ascii4Hex(checkval);
			LCD_string_write("\n");

			LCD_string_write(err);
			delay(5);

		}
		IOM = 1;
	}
	IOM = 0;
	LCD_string_write("\n");

	LCD_string_write(str2);
	LCD_CLEAR();
	
	return;
	
}
	

void Count() {
	LCD_CLEAR();
	__code unsigned char* newline = "\n";
	__code unsigned char* str = "Address?\n";
	LCD_string_write(str);
	
	__xdata unsigned int address = getKeyNum(4);
	LCD_string_write(newline);
	__xdata unsigned char* ram_address;
	ram_address = (__xdata unsigned char*) address;
	__code unsigned char* blockstr = "How many Blocks?\n";
	LCD_string_write(blockstr);
	__xdata unsigned int addrCount = getKeyNum(4);
	LCD_string_write(newline);
	__xdata unsigned long endAddr = address + addrCount;
	if (endAddr > 65535) 
	{
		endAddr = 65535;
	}
	
	
	__code unsigned char* valstr = "Value: \n";
	//LCD_string_write(valstr);
	//ascii4Hex(address);
	LCD_string_write(newline);
	
	__code unsigned char* editstr = "Enter value:\n";
	LCD_string_write(editstr);
	
	__xdata unsigned char findstore = getKeyNum(2);
	
	LCD_string_write(newline);
	
	__code unsigned char* countstr = "Count:";

	__xdata unsigned int num = (__xdata unsigned int)countLoop(findstore, address, endAddr, ram_address);
	LCD_string_write(countstr); 
	ascii4Hex(num);
	LCD_string_write(newline);
	findLoop(findstore, address, endAddr, ram_address, address); 
}
 void Find() {
	 LCD_CLEAR();
	__code unsigned char* newline = "\n";
	__code unsigned char* str = "Address?\n";
	LCD_string_write(str);
	
	__xdata unsigned int address = getKeyNum(4);
	LCD_string_write(newline);
	__xdata unsigned char* ram_address;
	ram_address = (__xdata unsigned char*) address;
	__code unsigned char* blockstr = "How many Blocks?\n";
	LCD_string_write(blockstr);
	__xdata unsigned int addrCount = getKeyNum(4);
	LCD_string_write(newline);
	__xdata unsigned long endAddr = address + addrCount;
	if (endAddr > 65535) 
	{
		endAddr = 65535;
	}
	
	
	__code unsigned char* valstr = "Value: \n";
	//LCD_string_write(valstr);
	//ascii4Hex(address);
	LCD_string_write(newline);
	
	__code unsigned char* editstr = "Enter value:\n";
	LCD_string_write(editstr);
	
	__xdata unsigned char findstore = getKeyNum(2);
	
	LCD_string_write(newline);
	findLoop(findstore, address, endAddr, ram_address, address); 
	
	
} 

 void findLoop(__xdata unsigned int findstore, __xdata unsigned int address, __xdata unsigned int endAddr, __xdata unsigned char* ram_address, __xdata unsigned int startAddr) {
	 unsigned char key;
	__xdata unsigned int i;
	__xdata unsigned char count = 0;
	__xdata unsigned char spaceCount = address;
	__xdata unsigned char lineCount = 0;
	__xdata unsigned int savedAddress = 0;
	__xdata unsigned char printedCount = 0;
	__code unsigned char* nextString = "\n0 next 1 back\n";
	__code unsigned char* space = " ";
	
	__code unsigned char* newline = "\n";
	__xdata unsigned char ramVal = 0;
	__xdata unsigned int a = 0;
	__code unsigned char* newline2 = ":\n";
	ascii4Hex(address);
	LCD_string_write(newline2);
	
	
	
	
			
	
	
	for (i = address; i <= endAddr; i++) {
		IOM = 0;
		
		ram_address = (__xdata unsigned char*)i;
		ramVal = (*ram_address);
		 if(i == endAddr || i == 0xFFFF) 
		{
			LCD_string_write(nextString);
			savedAddress = i;
			
			do {
				key = keyDetect();
			} while (key != '1' && key != 'E');
		
			if (key == '1') {
				LCD_CLEAR();
				if( i < (2 * (i - savedAddress))) 
				{
				a = 0;
				}
				else
				{
				a = i - (2 * (i - savedAddress));
				
				}
				if(a < startAddr)
				{
					a = startAddr;
				}
				findLoop(findstore, a, savedAddress - 1, ram_address, startAddr);
				return;
			}
			else if (key == 'E') {
				return;
			}
			return; 
		} 
		
		 if(ramVal == findstore)
		{
			ascii4Hex(i);
			count++;
			count++;
			count++;
			count++;
			
		
			LCD_string_write(space);
			spaceCount++;
		 if (lineCount < 6 && count + spaceCount >= 10) {
			LCD_string_write(newline);
			lineCount++;
			printedCount += count;
			count = 0;
			spaceCount = 0;
		} 
		else if (lineCount >= 6) {
			printedCount += count;
			
			LCD_string_write(nextString);
			
			 do {
				key = keyDetect();
			} while (key != '0' && key != '1' && key != 'E'); 
			
			
			LCD_string_write(&key);
			if(key == '0') {
				LCD_CLEAR();
				savedAddress = i;
			}
			else if (key == '1') {
				LCD_CLEAR();
				if( i < (2 * (i - savedAddress))) 
				{
				a = 0;
				
				}
				else
				{
				a = i - (2 * (i - savedAddress));
				}
				if(a < startAddr)
				{
					a = startAddr;
				}
				findLoop(findstore, a, savedAddress - 1, ram_address, startAddr);
				return;
			}
			else if (key == 'E') {
				return;
			}
			LCD_CLEAR();
			lineCount = 0;
			count = 0;
			spaceCount = 0;
		} 
		
		
		IOM = 1;
		}
		
		
	} 
	
} 
	
	
unsigned long countLoop(__xdata unsigned char findstore, __xdata unsigned int address, unsigned int endAddr, __xdata unsigned char* ram_address) {
	__xdata unsigned int i;
	__xdata unsigned long count = 0;
	
	
	
	 for (i = address; i <= endAddr; i++) {
		IOM = 0;
		ram_address = (__xdata unsigned char*)i;
		
		
		if(findstore == *ram_address)
		{
			count++;
		}
		
		if(i == endAddr) 
		{
			
			return count; 
		}
		IOM = 1;
	} 
	return count;
}
	
void LCD_CLEAR() {
	
fillScreen(BLACK);
setCursor(0,0);
}


long sizeCheck() {
	unsigned char firstkey;
	do {
	LCD_CLEAR();
	__code unsigned char* sizeString = "Size?\nB(1) W(2) DW(4)\n";
	LCD_string_write(sizeString);
	
	firstkey = keyDetect();
	LCD_string_write(&firstkey);
	} while (firstkey != '1' && firstkey != '2' && firstkey != '4');
	return firstkey;
}

long getDest() {
	
	__xdata float address;
	
	LCD_CLEAR();
	__code unsigned char* destString = "Destination?\n";
	LCD_string_write(destString);
	 address = getKeyNum(4);
	
	
	return address;
}
void Edit(__xdata unsigned int address, __xdata unsigned char* ram_address) {
	LCD_CLEAR();
	ram_address = (__xdata unsigned char*) address;
	__code unsigned char* str = "Current Value: \n";
	__code unsigned char* newline = "\n";
	LCD_string_write(str);
	ascii2Hex(*ram_address);
	LCD_string_write(newline);
	
	LCD_string_write(newline);
	
	
	__code unsigned char* editstr = "Enter new value:\n";
	LCD_string_write(editstr);
	LCD_string_write(newline);

	
	
	__xdata unsigned int editstore = getKeyNum(2);
	*ram_address = editstore;
	LCD_string_write(newline);
	__code unsigned char* editstr2 = "(0)cont (1)exit\n";
	LCD_string_write(editstr2);
	
	
	
	unsigned char key;
	
	do {
				key = keyDetect();
			} while (key != '0' && key != '1' && key != 'E');
			LCD_string_write(&key);
			
			
			
			if(key == '0') {
				
				LCD_CLEAR();
				if(address == 65535) {
					return;
				}
				Edit(address + 1, ram_address);
				return;
			} 
			else if (key == 1) {
				LCD_CLEAR();
				return;
			}
			else if (key == 'E') {
				LCD_CLEAR();
				return;
			}
}

void Move() {
	__xdata unsigned char* ram_address1;
	__xdata unsigned char* ram_address2;
	unsigned int firstkey;
	firstkey = sizeCheck();
	
	__xdata unsigned int numblocks;
	numblocks = getNumBlocks();
	numblocks *= firstkey;
	
	LCD_CLEAR();
	
	__code unsigned char* adrstr = "Address?\n";
	__code unsigned char* movstr = "Move Done!\n";
	LCD_string_write(adrstr);
	__xdata unsigned int address = getKeyNum(4);
	__xdata unsigned int addrCount = numblocks;
	
	__xdata unsigned int destination;
	destination = getDest(); 
	
	
	__xdata unsigned int endAddr = address + addrCount;
	
	
	__xdata unsigned int i; 
	ram_address2 = (unsigned char __xdata*)(destination);
	ram_address1 = (unsigned char __xdata*)(address);
	
	for (i = address; i <= endAddr; i++) {
		
		IOM = 0;
		*ram_address2 = *ram_address1;
		if(ram_address1 == (unsigned char __xdata*)endAddr || ram_address2 == (unsigned char __xdata*)endAddr) 
		{
			break;
		}
		if(i == 65535)
		{
			break;
		}
		ram_address1++;
		ram_address2++; 
		IOM = 1;
	}
	
	LCD_string_write(movstr);
	delay(200);
	
	
}

long getNumBlocks() {
	__xdata unsigned int numblocks;
	__code unsigned char* blkstr = "How many blocks?\n";
do {
	LCD_CLEAR();
	LCD_string_write(blkstr);
	numblocks = getKeyNum(4);
	} while (numblocks <= 0);
return numblocks;
}

void Dump() {
 

LCD_CLEAR();
__code unsigned char* dmpstr = "Address?\n";
__xdata unsigned char firstkey;
firstkey = sizeCheck();
__xdata unsigned int numblocks;
numblocks = getNumBlocks();

LCD_CLEAR();
LCD_string_write(dmpstr);
__xdata unsigned int address = getKeyNum(4);
__xdata unsigned int addrCount = numblocks * charToDec(firstkey);




dumpRAM(address, addrCount, firstkey, address);





}

int getKeyNum(int size) {
	__xdata unsigned int i;
	__xdata int num = 0;
	__xdata unsigned char key;
	for (i = 0; i < size; i++)
	{
		num = (num << 4);
		key = keyDetect();
		
		//LCD_string_write(&key);
		asciiToHex1(charToDec(key));
		num += charToDec(key);
		
		
	}
	

	return num;
}

int convertAddress(unsigned char high, unsigned char low) {
	high = charToDec(high);
	high = high * 16;
	low = charToDec(high);

	__xdata unsigned int address = high + low;
	return address;
}

int charToDec(unsigned char key) {
	if(key == '0')
	{
		return 0;
	}
	else if(key == '1')
	{
		return 1;
	}
	else if(key == '2')
	{
		return 2;
	}
	else if(key == '3')
	{
		return 3;
	}
	else if(key == '4')
	{
		return 4;
	}
	else if(key == '5')
	{
		return 5;
	}
	else if(key == '6')
	{
		return 6;
	}
	else if(key == '7')
	{
		return 7;
	}
	else if(key == '8')
	{
		return 8;
	}
	else if(key == '9')
	{
		return 9;
	}
	else if(key == 'A')
	{
		return 10;
	}
	else if(key == 'B')
	{
		return 11;
	}
	else if(key == 'C')
	{
		return 12;
	}
	else if(key == 'D')
	{
		return 13;
	}
	else if(key == 'E')
	{
		return 14;
	}
	else if(key == 'F')
	{
		return 15;
	}
	else return 0;
}

char decToChar(unsigned int d) {
	if (d < 10) {
		return d + 0x30;
	}
	else {
		return d + 0x31;
	}
	
}

void dumpRAM(__xdata unsigned int addr, __xdata unsigned int addrCount, __xdata unsigned char firstkey, __xdata unsigned int initialAdd){
	__xdata unsigned int i;
	__xdata unsigned int j = 0;
	__xdata unsigned char* ram_address = (__xdata unsigned char*)addr;
	__xdata unsigned int endAddr = addr + addrCount;
	__xdata unsigned int count = 0;
	__xdata unsigned int lineCount = 0;
	__xdata unsigned int spaceCount = 0;
	__xdata unsigned int printedCount = 0;
	__xdata unsigned char blockSize = charToDec(firstkey);
	__code unsigned char* newline = ":\n";
	__code unsigned char* space = " ";
	__code unsigned char* newline1 = "\n";
	__code unsigned char* str = "\n0 next 1 back\n";
	
	LCD_string_write(newline1);
		ascii4Hex(addr);
		LCD_string_write(newline);
		
		

	for (i = addr; i <= endAddr; i++) {
		IOM = 0;
		
		ram_address = (__xdata unsigned char*) i;
		if(i == endAddr || i == 0xFFFF)
		{
			printedCount += count;
			LCD_string_write(str);
			
			unsigned char key;
			do {
				key = keyDetect();
			} while (key != '1' && key != 'E');
			LCD_string_write(&key);
			
			if (key == '1') {
				LCD_CLEAR();
				
				__xdata unsigned int backAdd = addr - 31;
				if(addr < 31) 
				{
						backAdd = 0;
				}
				__xdata unsigned int forcount =  addrCount + (printedCount/2);
				if(backAdd < initialAdd  || addr - initialAdd < 31) 
				{
					backAdd = initialAdd;
				}
				
				dumpRAM(backAdd, forcount, firstkey, initialAdd);
				return;
			}
			else if (key == 'E') {
				return;
			}
		}
		
		else {
		ascii2Hex(*ram_address);
		
		
		count++;
		count++;
		addrCount -= 2;
		
		if (count % (2 * blockSize) == 0 ) {
			LCD_string_write(space);
			spaceCount++;
		
		 if (lineCount < 6 && count + spaceCount + (blockSize*2) >= 16) {
			LCD_string_write(newline1);
			lineCount++;
			printedCount += count;
			count = 0;
			spaceCount = 0;
		}
		
		
		else if (lineCount >= 6) {
			printedCount += count;
			LCD_string_write(str);
			unsigned char key;
			do {
				key = keyDetect();
			} while (key != '0' && key != '1' && key != 'E');
			LCD_string_write(&key);
			if(key == '0') {
				addr = i + 1;
				LCD_CLEAR();
				
			} 
			else if (key == '1') {
				LCD_CLEAR();
				
				__xdata unsigned int backAdd = addr - 31;
				if(addr < 31) 
				{
						backAdd = 0;
				}
				__xdata unsigned int forcount =  addrCount + (printedCount/2);
				if(backAdd < initialAdd  || addr - initialAdd < 31) 
				{
					backAdd = initialAdd;
				}
				
				dumpRAM(backAdd, forcount, firstkey, initialAdd);
				return;
			}
			
			else if (key == 'E') {
				return;
			}
			LCD_CLEAR();
			lineCount = 0;
			count = 0;
			spaceCount = 0;
			printedCount = 0;
		}
		}
		IOM = 1;
	}
	}

}

void printChar(char* val) {
	__xdata int i = 0;
	
	while (val[i] != '\0') 
	{
		write((u8)val[i]);
	}
}





void main (void) {
	CD = 0;
	IOM = 0;

	iowrite8(seg7_address, 0xF9);
	IOM = 0;
	CD = 1;
	TFT_LCD_INIT();
	//iowrite8(seg7_address, 0xF9);
	iowrite8(seg7_address, 0xA4);
	setCursor(0,0);
	setTextSize(2);
	setTextColor(CYAN, BLACK);
	IOM = 0;
	UART_Init();
	rtcInit();
	menuOperate();
	
  

	while(1) {
		
	}
}
