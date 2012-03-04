// modified from: http://www.windmeadow.com/node/38

// the SPI/SPI.h in v22 has a broken double #define SPI_CLOCK_DIV64
// this looks to be fixed with commit 965480f
#include <SPI.h>

#include <WProgram.h>

#define IN_BUF_SIZE 80

// IPIN_ is for pins that are inverted

// CS (chip select) is SS PB0 PIN 53 (through level shifter)
#define IPIN_CS 53

// SCLK (serial clock) is SCK PB1 PIN 52 (through level shifter)
#define PIN_SCLK 52

// DIN (data in) is MOSI PB2 PIN 51 (through level shifter)
#define PIN_DIN 51

// DOUT (data out) is MISO PB3 PIN 50 (direct)
#define PIN_DOUT 50

// CLKSEL (clock select) is PL0 PIN 49 (through level shifter)
#define PIN_CLKSEL 49

// START is PL3 PIN 46 (through level shifter)
#define PIN_START 46

// RESET is PL2 PIN 47 (through level shifter)
#define IPIN_RESET 47

// PWDN (power down) is PL1 PIN 48 (through level shifter)
#define IPIN_PWDN 48

// DRDY (data ready) is PL4 PIN 45 (direct)
#define IPIN_DRDY 45

// SDATAC Command (Stop Read Data Continuously mode)
#define SDATAC 0x11
#define RDATAC 0x10

// these are pulled from the ADS1298 data sheet
#define WREG 0x40
#define CONFIG1 0x01
#define CONFIG2 0x02
#define CONFIG3 0x03
#define PDREFBUF 0x80
#define CONFIG3DEF 0x40
#define VREF_4V 0x20
#define CHnSET 0x04		// CH1SET is 0x05, CH2SET is 0x06, etc.
// we need to set the RLD at some point

char byte_buf[IN_BUF_SIZE];
int pos;

extern "C" void __cxa_pure_virtual(void)
{
	// error - loop forever (nice if you can attach a debugger)
	while (true) ;
}

int main(void)
{
	char in_byte;
	int i, j;

	init();

	// set the LED on
	pinMode(13, OUTPUT);
	digitalWrite(13, HIGH);

	pinMode(IPIN_CS, OUTPUT);
	pinMode(PIN_SCLK, OUTPUT);
	pinMode(PIN_DIN, OUTPUT);
	pinMode(PIN_DOUT, INPUT);

	pinMode(PIN_CLKSEL, OUTPUT);
	pinMode(PIN_START, OUTPUT);
	pinMode(IPIN_RESET, OUTPUT);
	pinMode(IPIN_PWDN, OUTPUT);
	pinMode(IPIN_DRDY, INPUT);

	SPI.begin();

	SPI.setBitOrder(MSBFIRST);

	digitalWrite(IPIN_CS, LOW);
	digitalWrite(PIN_CLKSEL, HIGH);

	// Wait for 20 microseconds Oscillator to Wake Up
	delay(1);		// we'll actually wait 1 millisecond

	digitalWrite(IPIN_PWDN, HIGH);
	digitalWrite(IPIN_RESET, HIGH);

	// Wait for 33 milliseconds (we will use 1 second)
	//  for Power-On Reset and Oscillator Start-Up
	delay(1000);

	// Issue Reset Pulse,
	digitalWrite(IPIN_RESET, LOW);
	// actually only needs 1 microsecond, we'll go with milli
	delay(1);
	digitalWrite(IPIN_RESET, HIGH);
	// Wait for 18 tCLKs AKA 9 microseconds, we use 1 millisec
	delay(1);

	// Send SDATAC Command (Stop Read Data Continuously mode)
	SPI.transfer(SDATAC);

	// no external reference Configuration Register 3
	SPI.transfer(WREG | CONFIG3);
	SPI.transfer(0);	// number of registers to be read/written – 1
	SPI.transfer(PDREFBUF | CONFIG3DEF | VREF_4V);

	// Write Certain Registers, Including Input Short
	// Set Device in HR Mode and DR = fMOD/1024
	SPI.transfer(WREG | CONFIG1);
	SPI.transfer(0);
	SPI.transfer(0x86);	// TODO bitnames!
	SPI.transfer(WREG | CONFIG2);
	SPI.transfer(0);
	SPI.transfer(0x10);	// generate test signals
	// Set all channels to test signal
	for (i = 1; i <= 8; ++i) {
		SPI.transfer(WREG | (CHnSET + i));
		SPI.transfer(0);
		SPI.transfer(0x05);	// test signal
	}

	digitalWrite(PIN_START, HIGH);
	while (digitalRead(IPIN_DRDY) == HIGH) {
		continue;
	}

	Serial.begin(230400);

	// wait for a non-zero byte as a ping from the computer
	do {
		// loop until data available
		if (Serial.available() == 0) {
			continue;
		}
		// read an available byte:
		in_byte = Serial.read();
	} while (in_byte == 0);

	// Put the Device Back in Read DATA Continuous Mode
	SPI.transfer(RDATAC);

	while (1) {
		// wait for DRDY
		while (digitalRead(IPIN_DRDY) == HIGH) {
			;	// no data yet
		}
		pos = 0;

		byte_buf[pos++] = '[';
		byte_buf[pos++] = 'g';
		byte_buf[pos++] = 'o';
		byte_buf[pos++] = ']';

		// read 24bits of status then 24bits for each channel
		for (i = 0; i <= 8; ++i) {
			for (j = 0; j < 3; ++j) {
				in_byte = SPI.transfer(0);
				byte_buf[pos++] = 'A' + ((in_byte & 0xF0) >> 4);
				byte_buf[pos++] = 'A' + (in_byte & 0x0F);
			}
		}

		byte_buf[pos++] = '[';
		byte_buf[pos++] = 'o';
		byte_buf[pos++] = 'n';
		byte_buf[pos++] = ']';
		byte_buf[pos++] = '\n';

		Serial.print(byte_buf);
	}
}
