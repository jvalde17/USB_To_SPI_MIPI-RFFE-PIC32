//*********************************************************************
// Prototype Project to test SPI
// GPIO and UART comms
//
//*********************************************************************

/* Adds support for PIC32 Peripheral library functions and macros */
#include <plib.h>
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//****************************************************************
//
// Configure Port D 0 - 9 as input
// Configure Port E 0 - 7 as input
// Configure Port B2 as an output for the direction control
//
//****************************************************************
void Init_GPIO(void)
{
	unsigned int PINSD = 0x03FF;
	unsigned int PINSE = 0x00FF;
	unsigned int PINSB = 0x0004;
	unsigned int PINSG = 0x0140;
	
	PORTSetPinsDigitalIn(IOPORT_D, PINSD);
	PORTSetPinsDigitalIn(IOPORT_E, PINSE);
	PORTSetPinsDigitalOut(IOPORT_B, PINSB);
	PORTSetPinsDigitalOut(IOPORT_G, PINSG);
	TRISGCLR = 0x00000040;  // used for artificial clock (TA)
	LATGCLR = 0x00000040;
}	

//**************************************************************
//  Configure each port bit as an input or output
//  0 = output and 1 = input
//**************************************************************
void ConfigureGPIO(void)
{
	unsigned int i;
	
// Check length of command	
	if(TXcount != 8)
	{  //fes
		CommandAck(0);  // invalid command
		return;
	}	
	switch (TXBuf[1])
	{
		case 'D':  // Port D
		case 'd':
			i = AtoH(2,4) & 0X03FF;
			TRISD = i;
			CommandAck(1);  // successful command
			break;
		case 'E':  // Port E
		case 'e':
			i = AtoH(2,4) & 0X00FF;
			TRISE = i;
			CommandAck(1);  // successful command
			break;
		default:  //fes
			CommandAck(0);  // invalid command
			break;
	}	
}

//*************************************************************
//  Write data to the output port bits
//  Write to the latch so you do not have to worry about bit 
//  direction
//*************************************************************
void WriteGPIO(void)
{
	unsigned int i;
// Check length of command	
	if(TXcount != 8)
	{  //fes
		CommandAck(0);  // Invalid command
		return;
	}	
	switch (TXBuf[1])
	{
		case 'D':  //Port D
		case 'd':
			i = AtoH(2,4) & 0X03FF;
			LATD = i;
			CommandAck(1);
			break;
		case 'E':  // Port E
		case 'e':
			i = AtoH(2,4) & 0X00FF;
			LATE = i;
			CommandAck(1);
			break;
		case 'G':
		case 'g':
			if (SPIOn == 0)
			{
				i= AtoH(2,4) & 0x0140;
				LATG = i;
				CommandAck(1);
			}	
			else  //fes
				CommandAck(0);
			break;
		default:   //fes
			CommandAck(0);  // Invalid command
			break;
	}	
}

//*************************************************************
// Read the input port bits and mask off the output bits
//*************************************************************
void ReadGPIO(void)
{
	unsigned int i,j;
	char Response[32];
	
// Check length of command	
	if(TXcount != 4)
	{  //fes
		CommandAck(0);  // Invalid Command
		return;
	}	
	while(TXctr != 0);
	switch (TXBuf[1])
	{
		case 'D':  // Port D
		case 'd':
			i = TRISD; // the mask is the data direction
			j = PORTD;
			j = j & i;
			sprintf(Response, "%x",j);
			TXBuf[0] = 0;
			strcat(TXBuf,"D ");
			strcat(TXBuf,Response);
			strcat(TXBuf,"\r");
			Send();
			break;
		case 'E':  // Port E
		case 'e':
			i = TRISE; // the mask is the data direction
			j = PORTE;
			j = j & i;
			sprintf(Response, "%x",j);
			TXBuf[0] = 0;
			strcat(TXBuf,"E ");
			strcat(TXBuf,Response);
			strcat(TXBuf,"\r");
			Send();
			break;
		default:   //fes
			CommandAck(0);  // Invalid command
			break;
	}	
}			

