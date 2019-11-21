//*********************************************************************
// Prototype Project to test SPI
// GPIO and UART comms
//
//*********************************************************************

// application defines
#define SYS_FREQ		(80000000)


//Commands.c
void Decode(void);
void CommandAck(char);
unsigned int AtoH(int, int);
void Error(void);


// GPIO.c
void Init_GPIO(void);
void ConfigureGPIO(void);
void WriteGPIO(void);
void ReadGPIO(void);


// Main.c


// SPI.c
void Init_SPI(void);
void SPIcmd(void);
int SPIRead(void);
void SPIWrite(void);
void Read8(void);
void Write(void);
void ChangeRate(void);
void ChangePhase(void);
void DebugValue(long);
void Read16(void);
void WriteAck(char);
void ReadAck0(void);
void WriteInfo(char);

// UART.c
void Init_UART(void);
//void __ISR(_UART2_VECTOR, ipl2) IntUart1Handler(void);
void Send(void);

//BitBangPic.c
void Init_BITBANG(void);
void BITBANGcmd(void);
void BangSlotAndTriggerCmd(void);
int rffe_write_2byte(int, int, int, int);
void rffe_write_1byte(void); //extended
void rffe_write(int, int, int ); //standard 1 byte
void rffe_read(int, int); //standard 1 byte
int rffe_read_2byte(void);
void rffe_write_3byte(int, int, int, int, int, int, int);
void tDelay(unsigned int ); 
int calc_parity(unsigned int, int );
void debug_message(void);
void DebugReadValue(unsigned int);
void Return_Exception(int);
void run_loop_slot(void);

//Variables
extern char Rev[];
extern char RXBuf[];
extern char TXBuf[];
extern char crFlag;
extern unsigned char RXcount;
extern unsigned int TXcount;
extern unsigned int TXctr;
extern unsigned char Phase; 
extern unsigned char Rate;
extern unsigned char Mode;
extern unsigned char cBreak;
extern unsigned char Echo;
extern unsigned char SPIOn;
//extern static int _Hw[20], _Reg0[20], _Reg1[20], _B0[20], _B1[20], _B2[20], _B3[20]; 

extern char loopSlotTx;
