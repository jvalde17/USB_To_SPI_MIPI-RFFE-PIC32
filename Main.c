//*********************************************************************
// SP05082011 Firmware
// Main.c
// Version 11/20/2011
//*********************************************************************

/* Adds support for PIC32 Peripheral library functions and macros */
#include <plib.h>
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Configuration Bits
#pragma config FNOSC    = PRIPLL        // Oscillator Selection
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider (PIC32 Starter Kit: use divide by 2 only)
#pragma config FPLLMUL  = MUL_24        // PLL Multiplier
#pragma config FPLLODIV = DIV_1         // PLL Output Divider
#pragma config FPBDIV   = DIV_1         // Peripheral Clock divisor
#pragma config FWDTEN   = OFF           // Watchdog Timer
#pragma config WDTPS    = PS1           // Watchdog Timer Postscale
#pragma config FCKSM    = CSDCMD        // Clock Switching & Fail Safe Clock Monitor
#pragma config OSCIOFNC = OFF           // CLKO Enable
#pragma config POSCMOD  = HS            // Primary Oscillator
#pragma config IESO     = OFF           // Internal/External Switch-over
#pragma config FSOSCEN  = OFF           // Secondary Oscillator Enable
#pragma config CP       = OFF           // Code Protect
#pragma config BWP      = OFF           // Boot Flash Write Protect
#pragma config PWP      = OFF           // Program Flash Write Protect
#pragma config ICESEL   = ICS_PGx1      // ICE/ICD Comm Channel Select
#pragma config DEBUG    = ON            // Debugger Disabled for Starter Kit

// application variables
char Rev[14] = "3-12-2012B";  // version 
char RXBuf[128];
char TXBuf[512];
char crFlag = 0;  // 1 = cr has been received
char loopSlotTx=0;
unsigned char RXcount = 0;  // characters in RXBuf
unsigned int TXcount = 0;  // characters to send
unsigned int TXctr = 0;  // count for the character to send
//unsigned char Mode = 1;
unsigned char Echo = 0; //  0 = off 1 = on - echo commands
unsigned char SPIOn = 1; // 1 = spi on, 0 = spi off
//static int _Hw[20], _Reg0[20], _Reg1[20], _B0[20], _B1[20], _B2[20], _B3[20]; 
//*******************************************************
// Start of program
//*******************************************************
int main(void)
{
	int i;

// Initialization	
   	SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);
  
	Init_GPIO();
	Init_SPI();
	Init_UART();

	// configure multi-vectored mode
	INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
	// enable interrupts
	INTEnableInterrupts();

// Save this as a speed test
//	while(1)
//{
//	LATDINV = 0x0002;
//	SpiChnPutC(1,'A');
//	rData =SpiChnGetC(1);	
//}

	TXBuf[0] = 0;
	strcat(TXBuf, "Version  ");
	strcat(TXBuf, Rev);
	strcat(TXBuf, "\r");
	Send();

// Main Loop
	while(1)
	{
// Echos the command		
		if(crFlag == 1)
		{
			crFlag = 0;
			TXBuf[0] = 0;
			RXcount = 0;
			// move the data from the RX buffer to the TX buffer
			strcat(TXBuf,RXBuf);
			if(Echo == 1)
				Send();
			else
			{
				TXcount = strlen(TXBuf);
				TXctr = 0;
			}	
			Decode();
		}
      if (loopSlotTx==1)
        {
			run_loop_slot();
        } //end loopSlotTx
      
	}	
}
// End of program


