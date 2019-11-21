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

void Init_UART(void)
{
	UARTConfigure(UART2, UART_ENABLE_PINS_TX_RX_ONLY);
	UARTSetFifoMode(UART2, UART_INTERRUPT_ON_RX_NOT_EMPTY | UART_INTERRUPT_ON_TX_DONE);
	UARTSetLineControl(UART2, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE |
		UART_STOP_BITS_1);
	U2BRG = 0x0000144;  // 0x0144 Baud Rate = 19200
	UARTEnable(UART2, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX ));//| UART_TX));	

	INTClearFlag(INT_SOURCE_UART_TX(UART2));
	INTClearFlag(INT_SOURCE_UART_RX(UART2));

	INTEnable(INT_SOURCE_UART_RX(UART2), INT_ENABLED);
	INTSetVectorPriority(INT_VECTOR_UART(UART2), INT_PRIORITY_LEVEL_2);
	INTSetVectorSubPriority(INT_VECTOR_UART(UART2), INT_SUB_PRIORITY_LEVEL_0);
}


// UART 2 interrupt handler
void __ISR(_UART2_VECTOR, ipl2) IntUart1Handler(void)
{
	// Is this an RX interrupt?
	if(INTGetFlag(INT_SOURCE_UART_RX(UART2)))
	{
		// There must be 1 instruction between the test and the clear
		RXBuf[RXcount] = (char)U2RXREG;
		// Clear the RX interrupt flag
		INTClearFlag(INT_SOURCE_UART_RX(UART2));
		if(RXBuf[RXcount] == '\\')
		{
			cBreak = 1;
			RXBuf[0] = 0;
			RXcount = 0;
		}	
		if(RXBuf[RXcount++] == 0x0D)
		{
			crFlag = 1;
			RXBuf[RXcount++] = 0x0A;
			RXBuf[RXcount++] = 0;
		}	
	}
	
	// TX interrupt	
	if(INTGetFlag(INT_SOURCE_UART_TX(UART2)) )
	{
		if(TXctr < TXcount)
		{
			U2TXREG = TXBuf[TXctr++];
		}	
		else
		{
			// disable the transmitter
			U2STACLR = 0x00000400;
			IEC1CLR = 0x00000400;
			TXctr = 0;
		}	
		// There must be 1 instruction between the test and the clear
		INTClearFlag(INT_SOURCE_UART_TX(UART2));
	}	
}

void Send(void)
{
	TXcount = strlen(TXBuf);	
	// enable the transmitter
	// Begin sending the data in the buffer
	U2TXREG = TXBuf[TXctr++];
	IEC1SET = 0x00000400;
	U2STASET = 0x00000400;
}	

