//Bit bang source file
#include <plib.h>
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#define DIR BIT_2
#define DAT BIT_8
#define CLK BIT_6

#define PORT IOPORT_G

#define PINS BIT_8 | BIT_6 | BIT_9 | BIT_2


// Using Internal Clock of 8 Mhz
#define FOSC 8000000L
 
// Delay Function 12000000
#define _delay_us(x) { unsigned char us; \
us = (x)/(80000000/FOSC)|1; \
while(--us != 0) continue; }

int _Hw[20], _Reg0[20], _Reg1[20], _B0[20], _B1[20], _B2[20], _B3[20], _ndx=0, _sdelay[20], _loop=0, _nslots=0; 
void write_mipi_data(unsigned int mask, unsigned int data, int nbits);

//******************************************************************
// Parse Bitbang related commands
//******************************************************************
void BITBANGcmd(void)
{
    static int _hw, _reg, _b0, _b1; 

    loopSlotTx=0;
    //Init_BITBANG(); 
	switch (TXBuf[1])
	{
	//******************
	// WRITE DATA 2-byte, extended write
	//******************		
		case 'W':
		case 'w':
	        _hw = AtoH(2,2);
	        _reg = AtoH(5,2);	
			_b0 = AtoH(10,2); //BWxx,AA,ddDD
		    _b1 = AtoH(8,2);  //BWxx,AA,DDdd
     	rffe_write_2byte(_hw, _reg, _b0, _b1);
        break;
    //******************
	// WRITE DATA 1-byte, extended write
	//******************
       case 'X':
	   case 'x':
       rffe_write_1byte();
       break;
    //******************
	// READ DATA 2-byte, extended read
	//*****************		
		case 'R':
		case 'r':
     	rffe_read_2byte();
        break;

   	//******************
	// WRITE DATA 1-byte, standard, addr=5bits
	//******************		
		case 'B':
		case 'b':
	        _hw = AtoH(2,2);
	        _reg = AtoH(5,2);	
			_b0 = AtoH(8,2); //BWxx,AA,dd
     	rffe_write(_hw, _reg, _b0);
        break;
   
    //******************
	// READ DATA 1-byte, standard, addr=5bits
	//******************
       case 'G':
	   case 'g':
             _hw = AtoH(2,2);
	        _reg = AtoH(5,2);	
     	rffe_read(_hw, _reg);
        break;	

     default: break;
 	}   
}

void BangSlotAndTriggerCmd(void)
{
     static int _hw, _reg0, _reg1, _b0, _b1, _b2, _b3;  
     char arr_buf[128];
     //int z;
    // Init_BITBANG(); 

	switch (TXBuf[1])
	{
	//************
	// WRITE 3 slot config bytes plus a trigger byte
	//************		
		case 'W':
		case 'w':
 			_hw = AtoH(2,2);
	        _reg0 = AtoH(5,2);	
			_b0 = AtoH(8,2); //BWxx,AA,11dddd,
		    _b1 = AtoH(10,2);  //BWxx,AA,dd22dd,
            _b2 = AtoH(12,2);  //BWxx,AA,dddd33,
            _reg1 = AtoH(15,2); //BWxx,AA,dddd33,AA aka trigger address
            _b3 = AtoH(17,2);  //BWxx,AA,dddd33,AA44,
     	rffe_write_3byte(_hw,_reg0,_b0, _b1, _b2,_reg1,_b3); //slot config->3 slot,1 trigger
        break;
        //************
		// WRITE DATA, Indexed version, for looping
		//************		
		case 'L':
		case 'l':
            //First get index
            //BWxx,AA,dddd33,AA44,
            //012345678901234567890
            _ndx = AtoH(20,1);
        	_Hw[_ndx] = AtoH(2,2);
	        _Reg0[_ndx] = AtoH(5,2);	
			_B0[_ndx] = AtoH(8,2); //BWxx,AA,11dddd,
		    _B1[_ndx] = AtoH(10,2);  //BWxx,AA,dd22dd,
            _B2[_ndx] = AtoH(12,2);  //BWxx,AA,dddd33,
            _Reg1[_ndx] = AtoH(15,2); //BWxx,AA,dddd33,AA
            _B3[_ndx] = AtoH(17,2);  //BWxx,AA,dddd33,AA44, 
            _sdelay[_ndx] = AtoH(22,3);
            _loop=1;
           
        break;
        //************
		// START
		//************	
        case 'd':
		case 'D':
            loopSlotTx=1;
            _nslots = AtoH(3,2);
        break;
        //************
		// STOP
		//************	
        case 'b':
		case 'B':
             loopSlotTx=0;
        break;


     default: break;
 	}   
}

//dynamic slot with trigger
void run_loop_slot(void)
{
   int z;

			mPORTFSetBits(BIT_3); //RF3 ARB TRiGGER
            tDelay(1); 
            mPORTFClearBits(BIT_3);

           for (z=0;z<_nslots;z++) {
           			rffe_write_3byte(_Hw[z],_Reg0[z],_B0[z],_B1[z],_B2[z],_Reg1[z],_B3[z]); 
                   
                        if (z==(_nslots-1)) { //trigger at end of num slot
								mPORTGWrite(0x04); //RG2 ARB TRiGGER
			                    tDelay(1); //
			                    mPORTGWrite(0x0);		
                          }
                    
     				tDelay(_sdelay[z]);               
             }									
}



//configure port
void Init_BITBANG(void)
{
    SPIOn = 0;
	SPI2CONCLR = 0x00008000;  // SPI OFF   
    mPORTBSetBits(DIR); //SET High

	PORTSetPinsDigitalOut(PORT, PINS);  
    PORTSetPinsDigitalIn(PORT, BIT_7); 
   
    PORTSetPinsDigitalOut(IOPORT_F, BIT_3);  
 
}   

//WRITE ---> This is the actual CLK and DATA drive
//2-byte write
int rffe_write_2byte(int dev_addr, int reg_addr, int bData0, int bData1)
{
   int bw;
   unsigned int  mask_1, mipi_word_1, k;
   unsigned int  mipi_word_0, mask_0; 
   unsigned int  cmd_frame; 
   int cmd_par, addr_par, dat0_par, dat1_par; 

   cmd_frame = (dev_addr << 8) + 1; //0=read cmd, + 1 means BC of 2 bytes
   mask_0=0x200000;
   mask_1=0x20000;
   k=0x200000; //used for delay only           

  //Calculate parity bits 
   cmd_par = calc_parity(cmd_frame, 12); //x501
   addr_par = calc_parity(reg_addr, 8);    
   dat0_par = calc_parity(bData0, 8);
   dat1_par = calc_parity(bData1, 8);
   
   if ( (reg_addr > 0xbe) || (reg_addr < 0)) { Return_Exception(1); return -1; }                        		                
   
  //assemble complete MIPI waveform, data0 first
   mipi_word_0 = (cmd_frame<<10) + (cmd_par<<9) + (reg_addr<<1) + addr_par ;
   mipi_word_1 = (bData0<<10) + (dat0_par<<9) + (bData1 << 1) + dat1_par; 

   mPORTBSetBits(DIR); //SET High
    //SSC
   	mPORTGWrite(0x100); //DATA
    k>>=1;     k<<=1;     k>>=1; 
    mPORTGWrite(0x100); //DATA
    k>>=1;     k<<=1;
    mPORTGWrite(0x000);
    k>>=1;

    write_mipi_data(mask_0, mipi_word_0, 21); //word 1    
    write_mipi_data(mask_1, mipi_word_1, 17); //word 2

	//BUS PARK CYCLE 
     mask_1>>=1; //just a bit of delay
	 mPORTGWrite(0x040); //CLK, DRIVE 0
     k>>=1;      k<<=1; 
     mPORTGWrite(0x00); //CLK DOWN
     mPORTBClearBits(DIR);//HIZ
     DebugReadValue(mipi_word_0);
     return 0;
}

//WRITE 
//4-byte write
int rffe_write_4byte(int dev_addr, int reg_addr, int D0, int D1, int D2, int D3 )
{
   int bw;
   unsigned int mask_1, mipi_word[4], k;
   unsigned int  mipi_word_0, mask_0; 
   unsigned int  cmd_frame; 
   int cmd_par, addr_par, d0_pr, d1_pr, d2_pr, d3_pr; 

   cmd_frame = (dev_addr << 8) + 3; //0=read cmd, + 3 means BC of 4 bytes
   mask_0=0x200000;
   mask_1=0x20000;
   k=0x200000; //used for delay only           

  //Calculate parity bits 
   cmd_par = calc_parity(cmd_frame, 12); //x501
   addr_par = calc_parity(reg_addr, 8);    
   d0_pr = calc_parity(D0, 8);
   d1_pr = calc_parity(D1, 8);
   d2_pr = calc_parity(D2, 8);
   d3_pr = calc_parity(D3, 8);
   
   if ( (reg_addr > 0xbe) || (reg_addr < 0)) { Return_Exception(1); return -1; }                        		                
   
  //assemble complete MIPI waveform, data0 first
   mipi_word[0] = (cmd_frame<<10) + (cmd_par<<9) + (reg_addr<<1) + addr_par ;
   mipi_word[1] = (D0<<10) + (d0_pr<<9) + (D1 << 1) + d1_pr; 
   mipi_word[2] = (D2<<10) + (d2_pr<<9) + (D3 << 1) + d3_pr; 

   mPORTBSetBits(DIR); //SET High
    //SSC
   	mPORTGWrite(0x100); //DATA
    k>>=1;     k<<=1;     k>>=1; 
    mPORTGWrite(0x100); //DATA
    k>>=1;     k<<=1;
    mPORTGWrite(0x000);
    k>>=1;

    write_mipi_data(mask_0, mipi_word[0], 21); //word 1    
    write_mipi_data(mask_1, mipi_word[1], 17); //word 2
    write_mipi_data(mask_1, mipi_word[2], 17); //word 2


	//BUS PARK CYCLE 
     mask_1>>=1; //just a bit of delay
	 mPORTGWrite(0x040); //CLK, DRIVE 0
     k>>=1;      k<<=1; 
     mPORTGWrite(0x00); //CLK DOWN
     mPORTBClearBits(DIR);//HIZ
     DebugReadValue(mipi_word_0);
     return 0;
}

/* *************************************************
   void write_mipi_data(unsigned int mask, unsigned int data, int nbits)
   - This is the generic bit-bang operations used by all write commands. 
 ***************************************************/
void write_mipi_data(unsigned int mask, unsigned int data, int nbits)
{
   int b,d;

   for (b=nbits; b>-1; b--){
       if (data & mask)  { 
					   			mPORTGWrite(0x140);
				                d>>=1;
                                d<<=1;
                                mask>>=1;   
                                mPORTGWrite(0x100); //clk down, data up             
							  }
	   else {
								mPORTGWrite(0x40);
					            d>>=1; 
                                d<<=1;
                                mask>>=1;
                                mPORTGWrite(0x00); //clk down, data down 
							   }
	}
}

//READ ---> This is the actual CLK and DATA drive
//unsigned int mPORTGReadBits(unsigned int _bits); //pg 141
int rffe_read_2byte(void)
{
   int bw;
   unsigned int dword, dat, mask_0, mask_1, mipi_word_0, mipi_word_1, k;
   unsigned int mipi_addr, dev_addr, cmd_frame; 
   unsigned int bData0, bData1;
   unsigned int bitr, byte_r;
   int cmd_par, addr_par; 
   
 // Get the dev address	
    dev_addr = AtoH(2,2);
    cmd_frame = (dev_addr << 8) + (2<<4) +1; //(2<<4)=read cmd

   // Get the address	
	mipi_addr = AtoH(5,2);	
	if(TXBuf[4] != ',')
	{
		WriteAck(0);
		return;
	}
 
   if ( (mipi_addr > 0xbe) || (mipi_addr < 0)) { Return_Exception(1);
                          return -1; }            

   mask_0=0x200000;
   mask_1=0x20000;
   k=0x200000; //used for delay only 
             
  //Calculate parity bits 
   cmd_par = calc_parity(cmd_frame, 12); 
   addr_par = calc_parity(mipi_addr, 8);    
   
  //assemble complete MIPI waveform.
   mipi_word_0 = (cmd_frame<<10) + (cmd_par<<9) + (mipi_addr<<1) + addr_par ;
   byte_r = 0x0;
 
   mPORTBSetBits(DIR); //SET High

    //SSC
  	mPORTGWrite(0x100); //DATA
    k>>=1; 
    k<<=1; 
    k>>=1; 
    mPORTGWrite(0x100); //DATA
    k>>=1; 
    k<<=1;
    mPORTGWrite(0x000);
    k>>=1;

    //word 1
    write_mipi_data(mask_0, mipi_word_0, 21); //word 1    

   //BUS PARK CYCLE 
     mask_1>>=1; //does nothing, just a bit of delay
	 mPORTGWrite(0x040); //CLK     
     k>>=1;
     k<<=1; 
     mPORTGWrite(0x00); //CLK DOWN, STAY HIZ
     mPORTBClearBits(DIR);//HIZ

    //bit read cycles but still clocking
	for (bw=17; bw>-1; bw--){
								mPORTGWrite(0x040); //clk=0x40, 0x140=>debug only remove
       							k>>=1; //delay
     							k<<=1; //delay	 								       							
								bitr = mPORTGReadBits(BIT_7); //read/sample the line RG7                                
                                bitr = bitr>>7;
								mPORTGWrite(0x00); //clk down                                
								byte_r += (bitr<<bw);  //=bitr shifted with num bit position                               
	}

	//BUS PARK CYCLE 
     //mPORTBSetBits(DIR);
	 mPORTGWrite(0x040); //CLK, DRIVE 0
     k>>=1;
     k<<=1; 
     mPORTGWrite(0x00); //CLK DOWN, STAY HIZ
     mPORTBClearBits(DIR);//HIZ - Release the bus at CLK Down

    DebugReadValue(byte_r & 0x3FFFF);
    return 0;
}

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
//3byte command (slot config) + 1 byte command (trigger) 
//write 3-byte mipi slot config + 4th byte is trig reg
//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
void rffe_write_3byte(int dev_addr, int reg_addr, int b0, int b1, int b2, int trig_addr, int b3)
{
   int bw;
   unsigned int dword,  dat, mask_0, mask_1, mask_3, mipi_word_1, mipi_word_0, trig_word, k;
   unsigned int  cmd_frame; 
   int cmd_par, reg_addr_par, b0p, b1p, b2p, b3p; 
   unsigned int cmd_trig, cmd_trig_p, trig_addr_p;
 
   cmd_frame = (dev_addr << 8) + 2; //SASA0000BCBC -> 0000 write, bytecount=0002

   mask_0=0x200000;
   mask_1=0x4000000;
   //mask_3=0x40000000; //trigger mask
   mask_3 = 0x200000;
   k=0x200000; //used for delay only           

  //Calculate parity bits 
   cmd_par = calc_parity(cmd_frame, 12); //x502
   reg_addr_par = calc_parity(reg_addr, 8);    
   b0p = calc_parity(b0, 8);
   b1p = calc_parity(b1, 8);
   b2p = calc_parity(b2, 8);
   b3p = calc_parity(b3, 8);
   
  //assemble command 
   mipi_word_0 = (cmd_frame<<10) + (cmd_par<<9) + (reg_addr<<1) + reg_addr_par ;
   mipi_word_1 = (b0<<19) + (b0p<<18) + (b1 << 10) + (b1p<<9) + (b2 << 1) + b2p;
 
   mPORTBSetBits(DIR); //SET High

    //SSC
   	mPORTGWrite(0x100); //DATA
    k>>=1; 
    k<<=1; 
    k>>=1; 
    mPORTGWrite(0x100); //DATA
    k>>=1; 
    k<<=1;
    mPORTGWrite(0x000);

    //word 1
	for (bw=21; bw>-1; bw--){ 
    if (mipi_word_0 & mask_0)  { //data and clk
					   			mPORTGWrite(0x140);
                                k>>=1;
                                k<<=1;
                                mask_0>>=1;
                                mPORTGWrite(0x100); //clk down, data up        
							  }
	   else {//clk only
								mPORTGWrite(0x40);
                                k>>=1;
                                k<<=1;
                                mask_0>>=1; 
                                mPORTGWrite(0x00); //clk down, data down  
							   }
	}
    //word 2
	for (bw=26; bw>-1; bw--){
       if (mipi_word_1 & mask_1)  { 
					   			mPORTGWrite(0x140);
				                k>>=1;
                                k<<=1;
                                mask_1>>=1;   
                                mPORTGWrite(0x100); //clk down, data up             
							  }
	   else {
								mPORTGWrite(0x40);
					            k>>=1; 
                                k<<=1;
                                mask_1>>=1;
                                mPORTGWrite(0x00); //clk down, data down 
							   }
	}
	//BUS PARK CYCLE 
     mask_1>>=1; //just a bit of delay
	 mPORTGWrite(0x040); //CLK, DRIVE 0
     k>>=1; 
     k<<=1; 
     mPORTGWrite(0x00); //CLK DOWN

    /* >>>>>>>>>>>>>>>>>>>>>>
       TRIGGER REGISTER
     <<<<<<<<<<<<<<<<<<<<<<<*/
     //trigger frame
    cmd_trig = (dev_addr<<8) + (2<<5) + (trig_addr); //2 is the 010
    cmd_trig_p = calc_parity(cmd_trig, 12); //0x5xx
    
    //trig_addr_p = calc_parity(trig_addr, 8);
    //trig_word = (cmd_trig<<19) + (cmd_trig_p<<18) + (trig_addr<<10) + (trig_addr_p<<9) + (b3 << 1) + b3p;
    
    trig_word = (cmd_trig<<10) + (cmd_trig_p<<9) + (b3 << 1) + b3p;

    k>>=1; 
    k<<=1; 
    k>>=1; 
    //SSC
   	mPORTGWrite(0x100); //DATA
    k>>=1; 
    k<<=1; 
    k>>=1; 
    mPORTGWrite(0x100); //DATA
    k>>=1; 
    k<<=1;
    mPORTGWrite(0x000);

    //bw=30(extended), 21 (standard)
	for (bw=21; bw>-1; bw--){ 
    if (trig_word & mask_3)  { //data and clk
					   			mPORTGWrite(0x140);
                                k>>=1;
                                k<<=1;
                                mask_3>>=1;
                                mPORTGWrite(0x100); //clk down, data up        
							  }
	   else {//clk only
								mPORTGWrite(0x40);
                                k>>=1;
                                k<<=1;
                                mask_3>>=1; 
                                mPORTGWrite(0x00); //clk down, data down  
							   }
	}
	//BUS PARK CYCLE 
     mask_3>>=1; //just a bit of delay
	 mPORTGWrite(0x040); //CLK, DRIVE 0
     k>>=1; 
     k<<=1; 
     mPORTGWrite(0x00); //CLK DOWN, HIZ
     mPORTBClearBits(DIR);//HIZ

     //DebugReadValue(trig_word);

} //end Write_Slot_Trigger

//WRITE ---> This is the actual CLK and DATA drive
//1-byte write, maybe used for trigger
//Using extended write structure
void rffe_write_1byte(void)
{
   int bw;
   unsigned int dword,  dat, k;
   unsigned int reg_addr, dev_addr, cmd_frame; 
   unsigned int bData0;
   int cmd_par, addr_par, dat0_par, dat1_par; 
   unsigned long long mipi_word_0, mipi_word_1, mask_0, mask_1; 
 
   // Get the dev address, BXxx,AA,dd	
    dev_addr = AtoH(2,2);
    cmd_frame = (dev_addr << 8); 

   // Get the reg address	
	reg_addr = AtoH(5,2);	
	if(TXBuf[7] != ',')
	{
		WriteAck(0);
		return;
	}	
   // Get the data	
	bData0 = AtoH(8,2); //BXxx,AA,dd

    mask_0=0x200000;
    mask_1=0x100;
    k=0x200000; //used for delay only                 

  //Calculate parity bits 
   cmd_par = calc_parity(cmd_frame, 12); //0x5xx
   addr_par = calc_parity(reg_addr, 8);    
   dat0_par = calc_parity(bData0, 8);

//assemble complete MIPI waveform, data0 first
   mipi_word_0 = (cmd_frame<<10) + (cmd_par<<9) + (reg_addr<<1) + addr_par ;
   mipi_word_1 =  (bData0 << 1) + dat0_par; 

    mPORTBSetBits(DIR); //SET High
    //SSC
   	mPORTGWrite(0x100); //DATA
    k>>=1; 
    k<<=1; 
    k>>=1; 
    mPORTGWrite(0x100); //DATA
    k>>=1; 
    k<<=1;
    mPORTGWrite(0x000);
    k>>=1;

    write_mipi_data(mask_0, mipi_word_0, 21); //word 1        
    write_mipi_data(mask_1, mipi_word_1, 8); //word 2    

	//BUS PARK CYCLE 
     mask_1>>=1; //just a bit of delay
	 mPORTGWrite(0x040); //CLK, DRIVE 0
     k>>=1; 
     k<<=1; 
     mPORTGWrite(0x00); //CLK DOWN
     mPORTBClearBits(DIR);//HIZ
}

//standard write, 1 byte
//address is limited to 5 bits (or masked 0x1f)
void rffe_write(int dev_addr, int reg_addr, int bData0)
{
   int bw;
   unsigned int mask_0,  mipi_word_0,  k;
   unsigned int cmd_frame; 
   int cmd_par, dat0_par; 

    cmd_frame = (dev_addr<<8) + (2<<5) + (reg_addr); //2 is the 010
    mask_0=0x200000;
    k=0x20000; //used for delay only          

  //Calculate parity bits 
   cmd_par = calc_parity(cmd_frame, 12);   
   dat0_par = calc_parity(bData0, 8);

   //assemble command 
    mipi_word_0 = (cmd_frame<<10) + (cmd_par<<9) + (bData0 << 1) + dat0_par;

    mPORTBSetBits(DIR); //SET High
    //SSC
   	mPORTGWrite(0x100); //DATA
    k>>=1; 
    k<<=1; 
    k>>=1; 
    mPORTGWrite(0x100); //DATA
    k>>=1; 
    k<<=1;
    mPORTGWrite(0x000);
    k>>=1;

    write_mipi_data(mask_0, mipi_word_0, 21); //word 1    

	//BUS PARK CYCLE 
     mask_0>>=1; //just a bit of delay
	 mPORTGWrite(0x040); //CLK, DRIVE 0
     k>>=1; 
     k<<=1; 
     mPORTGWrite(0x00); //CLK DOWN
     mPORTBClearBits(DIR);//HIZ
}

void rffe_read(int dev_addr, int reg_addr)
{
   int bw, br;
   unsigned int  mask_0,  mipi_cmd,  k;
   unsigned int cmd_frame; 
   int cmd_par;
   unsigned int bitr=0, byte_r=0; 

    cmd_frame = (dev_addr<<8) + (3<<5) + (reg_addr); //3 is the 011 cmd (read)
    mask_0=0x1000;
    k=0x20000; //used for delay only          

  //Calculate parity bits 
   cmd_par = calc_parity(cmd_frame, 12);   

   //assemble command 
    mipi_cmd = (cmd_frame<<1) + cmd_par;
    mPORTBSetBits(DIR); //SET High

    //SSC
   	mPORTGWrite(0x100); //DATA
    k>>=1; 
    k<<=1; 
    k>>=1; 
    mPORTGWrite(0x100); //DATA
    k>>=1; 
    k<<=1;
    mPORTGWrite(0x000);
    k>>=1;

    write_mipi_data(mask_0, mipi_cmd, 12); //read cmd

	//BUS PARK CYCLE 
     mask_0>>=1; //does nothing, just a bit of delay
	 mPORTGWrite(0x40); //CLK
     k>>=1;
     k<<=1; 
     mPORTGWrite(0x00); //CLK DOWN
     mPORTBClearBits(DIR);//HIZ - Release the bus at CLK Down
     k>>=1;
     k<<=1; 

    //Start reading
	for (br=8; br>-1; br--){
								mPORTGWrite(0x040); //clk=0x40, 0x140=>debug only remove
       							k>>=1; //delay
     							k<<=1; //delay								       							
								bitr = mPORTGReadBits(BIT_7); //read/sample the line RG7                               
                                bitr = (bitr>>7);     //need shift 7 because i always get a byte
								mPORTGWrite(0x00); //0x00 clking down                                 
								byte_r += (bitr<<br);  //=bitr shifted with num bit position                               
	}

	//BUS PARK CYCLE 
    // mPORTBSetBits(DIR);
	 mPORTGWrite(0x040); //CLK, DRIVE 0
     k>>=1;
     k<<=1; 
     mPORTGWrite(0x00); //CLK DOWN, STAY HIZ
     mPORTBClearBits(DIR);//HIZ - Release the bus at CLK Down

    DebugReadValue(byte_r & 0x1FF);
}

//500 Usec Delay function 
void tDelay(unsigned int del) 
{ 
  unsigned int z;
     for (z=0;z<del;z++) _delay_us(100); 
} 

int calc_parity(unsigned int i_bin, int nbits)
{
	unsigned int iparity, mask=0x1;
	int x, dat;
	
	mask<<=(nbits-1);
	
	dat = (i_bin & mask) >> (nbits-1); //initial
	mask>>=1;
	
	for (x=(nbits-2);x>-1;x--)
	 {
	 	dat = dat ^ ((i_bin & mask) >> x);
		mask>>=1;
	 }
	 
	iparity = (~dat & 0x1) ; //odd parity
	return iparity;
}

//**************************************************
// Jess' message
//**************************************************
void debug_message(void)
{
	while(TXctr != 0);
	TXBuf[0] = 0;
	strcat(TXBuf, ">>>>>>>>>>>>\r");
	Send();
}

//*******************************************
// Used for debugging
// Sends out data
//*******************************************
void DebugReadValue(unsigned int a)
{
	char Value[32];
	
	while(TXctr != 0);	
	TXBuf[0] = 0;
   
    sprintf(Value, "%.5x", a);
	strcat(TXBuf, "RW ");
	strcat(TXBuf, Value);
	strcat(TXBuf, "\n\r");
    //sprintf(TXBuf, "RW %.5x\n\r", Value);
	Send();
}

void Return_Exception(int a)
{
	while(TXctr != 0);	
	TXBuf[0] = 0;

    switch (a) {
    case 1:    strcat(TXBuf, "Addr Error"); break;
    case 2:    strcat(TXBuf, "Data Error"); break;
    case 3:    strcat(TXBuf, "USID Error"); break;
    default: strcat(TXBuf, "Cmd Error");   break; 
				}

	strcat(TXBuf, "\n\r");
	Send();
}