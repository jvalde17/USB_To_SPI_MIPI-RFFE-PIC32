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

#define DIR BIT_2

unsigned char cPreamble = 0x0B;  // Read = 0x0B
unsigned char cAddress = 0x0;
unsigned char cData0 = 0x55;
unsigned char cData1 = 0x55;
unsigned char cAck = 0;
unsigned char rData[5];
unsigned char Phase, Rate;
unsigned int urData[4];
unsigned char cBreak = 0;

void DelayMs(unsigned int msec)
{
	unsigned int tWait, tStart;
		
    tWait=(SYS_FREQ/2000)*msec;
    tStart=ReadCoreTimer();
    while((ReadCoreTimer()-tStart)<tWait);		// wait for the time to pass
}

//******************************************************************
// Initialize the SPI
//******************************************************************
void Init_SPI(void)
{
	IEC1CLR = 0x03800000; 	// disable all interrupts
	SPI2CON = 0;			// Stop and reset SPI
	IFS1CLR = 0x03800000;	// clear any existing event
	SPI2BRG =5;				// 8MHz
	Rate = 5;
	SPI2STATCLR = 0x40;		// clear the overflow
	//SPI2CON = 0x00008360;	// SPI ON, 8 bit transfers, SMP = 1, Master
    SPI2CON = 0x00008220;
	Phase = 1;

    PORTSetPinsDigitalOut(IOPORT_G,  BIT_9); //New Dir
    PORTSetPinsDigitalOut(IOPORT_G,  BIT_8); //data

    PORTSetPinsDigitalOut(IOPORT_B,  DIR);
    PORTSetPinsDigitalOut(IOPORT_E,  BIT_0);//ss
    mPORTESetBits(BIT_0); //SET High
}

//******************************************************************
// Parse SPI related commands
//******************************************************************
void SPIcmd(void)
{
	char Response[32];
	int i;
	unsigned long Data;

	switch (TXBuf[1])
	{
//************
// READ DATA
//************		
		case 'R':
		case 'r':
			if(SPIOn == 0)
			{
				ReadAck0();
				return;
			}	
			i = SPIRead();
			if(i == 0)  // Error?
				return;
			Data = rData[0];

			sprintf(Response, "%x", Data);
			while(TXctr != 0);
			TXBuf[0] = 0;
			strcat(TXBuf, "SR 1, ");
			strcat(TXBuf, Response);
			strcat(TXBuf, "\r");
			Send();
			break;
//************
// WRITE DATA
//************		
		case 'W':
		case 'w':
			if(SPIOn == 0)
			{
				//WriteAck(0);
                WriteInfo(0);
				break;;
			}	
			SPIWrite();
			break;
//***************************
// INFINITE LOOP FOR TESTING
//***************************
		case 'L':
		case 'l':
			if(SPIOn == 0)
			{
				CommandAck(0); //fes
				return;
			}	
			while(1)
			{
				Read16();
//			Write();
				DelayMs(1);
				if (cBreak == 1)
				{
					cBreak = 0;
					break;	
				}	
			}
			break;
		case 'O':  // Turn on SPI
			SPIOn = 1;
			SPI2CONSET = 0x00008000;  // SPI ON
//Set direction for receive
			mPORTBSetBits(DIR);
			CommandAck(1);
			break;
		case 'o': // Turn off SPI
			SPIOn = 0;
			SPI2CONCLR = 0x00008000;  // SPI OFF
//Set direction for transmit
			mPORTBClearBits(DIR);
			CommandAck(1);
			break;
		default:
			CommandAck(0); //fes
			break;
	}	
}	


//************************************************************
// Read 1 address using SPI
// Command is SR Preamble - 2 characters
// Comma
// Address - 2 characters
//
// SRpp,aa
//
//************************************************************
int SPIRead(void)
{
	int i;
	
// Is the command the correct length	
/*	i = strlen(TXBuf);
	if (i != 9)
	{  // No
		WriteInfo(0);
		return 0;
	}	*/
	
// Get the address	
	cAddress = (unsigned char)AtoH(5,2);	
	Read8();
	return 1;
}

//************************************************************
// Write 1 address using SPI
// Command is SW Preamble - 2 characters
// Comma
// Address - 2 characters
// Comma
// Data - 4 characters
//
// SWpp,aa,dddd
//
//************************************************************
void SPIWrite(void)
{
//	int i;
// Is the command the correct length?	
/*	i = strlen(TXBuf);
	if (i != 10)
	{  // No
		WriteInfo(i);
		return;
	} */	
	
// Get the address	
	cAddress = (unsigned char)AtoH(5,2);	
	if(TXBuf[7] != ',')
	{
		WriteInfo(1);
		return;
	}	
// Get the data	
	cData1 = (unsigned char)AtoH(8,2);
	Write();
	WriteInfo(8);
}		


//******************************************************
// This writes data out the SPI
//******************************************************
void Write(void)
{
	SPI2STATCLR = 0x00000001;
	IFS1CLR = 0x0380000;

//Set direction for transmit
      mPORTGWrite(0x00); //(RG9)
      mPORTBClearBits(DIR);  //SS, kills data if I remove?
      mPORTEClearBits(BIT_0);  //SS
      mPORTBClearBits(BIT_8); 

// WRITE - Send cmd phase, address phase and data phase
	SpiChnPutC(2,0x40);
	SpiChnPutC(2,cAddress);
	SpiChnPutC(2,cData1);
		
// Wait for end of transmission
	while(SPI2STAT & 0x00000800);

    SPI2STATCLR = 0x00000841;
    SPI2CONCLR = 0x00008000;  // SPI OFF
   //SPI2CONSET = 0x00008000;  // SPI ON

    mPORTBClearBits(BIT_8); //Drive data to 0 per Sean's Spec
    mPORTGWrite(0x200); //HIZ
    mPORTGSetBits(DIR); //kills data if I remove?
    mPORTESetBits(BIT_0); //SET High

    SPI2CONSET = 0x00008000;  // SPI ON
}

//**************************************************
// This reads the data in 8 bit chuncks
//**************************************************
void Read8(void)
{
   	{  
		SPI2STATCLR = 0x00000001;
		IFS1CLR = 0x03800000;

//Set direction for transmit
         mPORTGWrite(0x00); //(RG9)
         mPORTBClearBits(DIR);  //DIR
         mPORTEClearBits(BIT_0);  //SS

// WRITE - Send preamble and address
		SpiChnPutC(2,0x60);
		SpiChnPutC(2,cAddress);
		
// Wait for end of transmission
		while(SPI2STAT & 0x00000800);
		SPI2STATCLR = 0x00000841;
		
// Change direction for receive		
        mPORTGWrite(0x200); //dir of G9
		rData[0] = SPI2BUF;  //dummy read

        SpiChnPutC(2,0x02);  //dummy write to keep clking
        rData[0] = getcSPI2();
		}
        
        mPORTESetBits(BIT_0); //SET SS High
        mPORTBClearBits(BIT_8); //Drive data to 0 per Sean's Spec
}

//*************************************************************
// Change the SPI clock / data phase relationship
//*************************************************************
void ChangePhase(void)
{
	switch(TXBuf[1])
	{
		case '1':
			SPI2CON = 0;
			SPI2CON = 0x00008220;
			LATGSET = 0x00000040;
			Phase = 1;
			break;
		case '2':
			SPI2CON = 0;
			SPI2CON = 0x00008260;
			LATGCLR = 0x00000040;
			Phase = 2;
			break;
		case '3':
			SPI2CON = 0;
			SPI2CON = 0x00008320;
			LATGSET = 0x00000040;
			Phase = 3;
			break;
		case '4':
			SPI2CON = 0;
			SPI2CON = 0x00008360;
			LATGCLR = 0x00000040;
			Phase = 4;
			break;
		default:
			break;
	}	
}			

//***********************************************************************
// Change the SPI bit rate
//***********************************************************************
void ChangeRate(void)
{
	switch(TXBuf[1])
	{
		case '1':
			SPI2BRG =1;  // 24MHz
			Rate = 1;
			break;
		case '2':
			SPI2BRG =2;  // 16MHz
			Rate = 2;
			break;
		case '3':
			SPI2BRG =3;  //12MHz
			Rate = 3;
			break;
		case '4':
			SPI2BRG =4;  // 9.6MHz
			Rate = 4;
			break;
		case '5':
			SPI2BRG =5;  // 8MHz
			Rate = 5;
			break;
		case '6':
			SPI2BRG =6;  // 6.9MHz
			Rate = 6;
			break;
		case '7':
			SPI2BRG =7;  // 6MHz
			Rate = 7;
			break;
		case '8':
			SPI2BRG =8;  // 5.3MHz
			Rate = 8;
			break;
		case '9':
			SPI2BRG =9;  // 4.8MHz
			Rate = 9;
			break;
		default:
			break;
	}	
}	

//*******************************************
// Used for debugging
// Sends out data
//*******************************************
void DebugValue(long a)
{char Value[32];
	
	while(TXctr != 0);
	sprintf(Value, "%x", a);
	TXBuf[0] = 0;
	strcat(TXBuf, "DV ");
	strcat(TXBuf, Value);
	strcat(TXBuf, "\r");
	Send();
}	

//**************************************************
// This reads the data in 16 bit chuncks
//**************************************************
void Read16(void)
{
	unsigned int a;
	
// Change to 16 bit mode
	SPI2CONSET = 0x00000400;

// Preamble and address
	a = (cPreamble << 8) + cAddress;
//	DebugValue(a);
	SPI2STATCLR = 0x00000001;
	IFS1CLR = 0x03800000;

//Set direction for transmit
	mPORTBClearBits(DIR);
// WRITE - Send preamble and address
	SpiChnPutC(2,a);
		
// Wait for end of transmission
	urData[0] = getcSPI2();
		
// Change direction for receive		
	mPORTBSetBits(DIR);

// Create an artificial clock pulse
	SPI2CONCLR = 0x00008000;  // SPI OFF
	SPI2CONSET = 0x00008000;  // SPI ON
		
// Get acknowledge		
	SpiChnPutC(2,0);
	SpiChnPutC(2,0);
	urData[0] = getcSPI2();
	urData[1] = getcSPI2();
//	DebugValue(urData[0]);
//	DebugValue(urData[1]);
// Create an artificial clock pulse
	SPI2CONCLR = 0x00008000;  // SPI OFF
	SPI2CONSET = 0x00008000;  // SPI ON
	
	// Change to 8 bit mode
	SPI2CONCLR = 0x00000400;
}	

//**************************************************
// SPI Write Ack
//**************************************************
void WriteAck(char c)
{
	while(TXctr != 0);
	TXBuf[0] = 0;
	if(c == 0)
		strcat(TXBuf, "ACK 0\r");
	else
		strcat(TXBuf, "ACK 1\r");
	Send();
	
}

//**************************************************
// debug info
//**************************************************
void WriteInfo(char c)
{
	while(TXctr != 0);
	TXBuf[0] = 0;

     sprintf(TXBuf, "Seq %d\r", c);
	Send();	
}	

//**************************************************
// SPI Read Ack 0
//**************************************************
void ReadAck0(void)
{
	while(TXctr != 0);
	TXBuf[0] = 0;
	strcat(TXBuf, "SR 0, 0\r");
	Send();
}	
