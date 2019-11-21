//*********************************************************************
// 
// Decode the commands
// Version 11/20/2018
// By Jess Valdez
//*********************************************************************

/* Adds support for PIC32 Peripheral library functions and macros */
#include <plib.h>
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


void Decode(void)
{
	char a[32];
	
// Decode which command
	switch (TXBuf[0])
	{
		case 'C':  // Configure GPIO Direction
		case 'c':
			if(strlen(TXBuf) != 8)
				Error();
			else	
				ConfigureGPIO();  // Set bits as I or O
			break;

		case 'W':  // Write GPIO data
		case 'w':
			if(strlen(TXBuf) != 8)
				Error();
			else	
				WriteGPIO();  // Write data to the output bits
			break;

		case 'R':  // Read GPIO data
		case 'r':
			if(strlen(TXBuf) != 4)
				Error();
			else	
				ReadGPIO();  // Read data from the input bits
			break;

		case 'S':  // Send or receive SPI data
		case 's':
			SPIcmd();  // Read or write SPI data
			break;

		case 'K':  // Change the bit rate of the SPI
		case 'k':
			if(strlen(TXBuf) != 4)
				Error();
			else
				ChangeRate();
			break;
		case 'P':  // Change the SPI phase
		case 'p':
			if(strlen(TXBuf) != 4)
				Error();
			else
				ChangePhase();
			break;
		case 'F':  // displays SPI setup
		case 'f':
			while(TXctr != 0);
			TXBuf[0] = 0;
			sprintf(a,"%d",Phase);
			strcat(TXBuf, "Phase " );
			strcat(TXBuf, a);
			sprintf(a, "%d", Rate);
			strcat(TXBuf,"  Rate " );
			strcat(TXBuf ,a);
			if(Echo == 1)	strcat(TXBuf, "  Echo ON");
			else 			strcat(TXBuf, "  Echo OFF");
			if(SPIOn == 1)	strcat(TXBuf, "  SPI ON");
			else			strcat(TXBuf, "  SPI OFF");
			strcat(TXBuf,"\r");
			Send();
			break;
		case 'E':  // enable echoing commands
			Echo = 1;
			CommandAck(1);
			break;
		case 'e':  // disable echoing commands
			Echo = 0;
			CommandAck(1);
			break;
        case 'B':  
        case 'b':  //Bit Bang CLK and Data pins MIPI
            BITBANGcmd();
            TXBuf[0] = '\0';
			break;
        case 'T':  
        case 't':  //3 byte mipi with trigger
            BangSlotAndTriggerCmd();
			break;
        case 'M':  //MIPI Setup /Configure
        case 'm':
            Init_BITBANG();
            TXBuf[0] = '\0';
            break;
		case '?':
			strcpy(TXBuf, "Version  ");
			strcat(TXBuf, Rev);
			strcat(TXBuf, "\r");
			strcat(TXBuf, "nnnn - hex EX: For 0x01DC - nnnn = 01DC\r\r");
			strcat(TXBuf, "  GPIO\r");
			strcat(TXBuf, "cdnnnn or CDnnnn - configure Port D - 1 = input & 0 = output\r");
			strcat(TXBuf, "  Bits D0 to D9\r");
			strcat(TXBuf, "cennnn or CEnnnn - configure Port E\r");
			strcat(TXBuf, "  Bits E0 to E7\r");
			strcat(TXBuf, "rd or RD - read Port D\r");
			strcat(TXBuf, "re or RE - read Port E\r");
			strcat(TXBuf, "wdnnnn or WD - write to Port D\r");
			strcat(TXBuf, "wennnn or WE - write to Port E\r");
			strcat(TXBuf, "wgnnnn orWGnnnn - write to port G\r");
			strcat(TXBuf, "  Bits G6 and G8\r");
			strcat(TXBuf, "  SPI\r");
			strcat(TXBuf, "so or So - turn off SPI\n\r");
			strcat(TXBuf, "sO or SO - turn on SPI\n\r");
			strcat(TXBuf, "srpp,aa or SRpp,aa - read data\n\r");
			strcat(TXBuf, "  pp - preamble - typically 0A, aa- address\n\r");
			strcat(TXBuf, "swpp,aa,nnnn or SWpp,aa,nnnn - write data\n\r");
			strcat(TXBuf, "sl or SL - continuous loop - SPI read every 1 ms\n\r");
			strcat(TXBuf, "  '\\' ends the loop\r\r");
			strcat(TXBuf, "  Misc Commands\n\r");
			strcat(TXBuf, "E - command echo on, e - command echo off\n\r");
			strcat(TXBuf, "f or F - show status\n\r");
			strcat(TXBuf, "kn or Kn - change SPI data clock frequency\n\r");
			strcat(TXBuf, "  n = 1 to 9 - 1 = 24MHZ and 9 = 4.8MHz\r");
			strcat(TXBuf, "pn or Pn - change the SPI data / clock phase.  n = 1 to 4\r");
            strcat(TXBuf, "BRpp,aa or BWpp,aa,nnnn - 2-byte mipi read/write)\n\r");
            strcat(TXBuf, "TWpp,aa,112233,AA44 - mipi slot config data plus trigger\n\r");
            strcat(TXBuf, "TLpp,aa,112233,AA44,II - Load,II=Index\n\r");
			Send();
			break;
		default:  //fes
			CommandAck(0);  // Invalid command
			break;
	}	
}	

//*******************************************
// Used to acknowledge a command
// 1 = success
// 0 = failure
//*******************************************
void CommandAck(char c)
{
	char i[20];
	while(TXctr != 0);
	TXBuf[0] = 0;
	sprintf(i, "%d", c);
	strcat(TXBuf,i);
	strcat(TXBuf,"\r");
	Send();
}

//*******************************************************
// Convert ASCII to HEX
// Start at location n for m characters
// return unsigned int
//*******************************************************
unsigned int AtoH(int n,int m)	
{
	unsigned int i,j;
	unsigned char k; 
	
	j = 0;
	for (i = n ; i < n + m ; i++)
	{
		j = j << 4;
		k = TXBuf[i];
		if((k > 0x2F) && (k < 0x3A))
			k = k - 0x30;
		else if((k > 0x40) && (k < 0x47))  // caps
			k = k -  0x37;
		else if((k > 0x60) && (k < 0x67))  // lower case
			k = k - 0x57;
		else
		{
			j =  0xFF;  // command error
			break;
		}
		j = j + (unsigned int)k;
	}	
	return j;
}

//**********************************************
// Displays error message
//**********************************************
void Error(void)
{
	while(TXctr != 0);
	TXBuf[0] = 0;
	strcpy(TXBuf, "Incorrect comand length\n\r");
	Send();
}	


