

#include "tm4c123gh6pm.h"
#include "screen.h"

#define FRAME_REFRESH_HZ 	11
#define LCD_REFRESH_HZ 	  44
#define BUFFER_SIZE 					(SCREEN_W*SCREEN_H/8)

#define SerialData_PF04 (*((volatile unsigned long *)0x40025044))
#define DataClk_PF1 (*((volatile unsigned long *)0x40025008))
#define LatchClk_PF2 (*((volatile unsigned long *)0x40025010))
#define FrameClk_PF3 (*((volatile unsigned long *)0x40025020))


static void SwitchBuffer(void);
//static void DataClock(void);
static void LatchClock(void);
static void FrameClock(void);
static void PrintNextRow(uint8_t rowIndex);

uint8_t ScreenBufferA[BUFFER_SIZE] = {0};
uint8_t ScreenBufferB[BUFFER_SIZE] = {0}; 
uint8_t *writeBuffer = NULL;
uint8_t *readBuffer = NULL;

bool writeBufferIsFull = false;
bool writeBufferAvailable = true;

/*************************************/
/********* Public Functions **********/
/*************************************/

void Screen_Init(void){
 	//////////////////////////////////////////////////
	//Enable GPIOs for connecting to LCD            //
	//////////////////////////////////////////////////
	if((SYSCTL_RCGC2_R & (1<<5)) == 0) 								//If not yet active
	{
		SYSCTL_RCGC2_R |= (1<<5); 											// activate port F clock
		while((SYSCTL_PRGPIO_R & (1<<5)) == 0){};				// Wait for clock to stabalize 
	}	
	GPIO_PORTF_LOCK_R = 0x4C4F434B;   // unlock PortF PF0  
  GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF4-0
  GPIO_PORTF_DIR_R 		|= 	 ((1<<4)|(1<<3)|(1<<2)|(1<<1)|(1<<0));     // make PF4-0 out (1)
	GPIO_PORTF_AFSEL_R 	&= 	~((1<<4)|(1<<3)|(1<<2)|(1<<1)|(1<<0));  		// disable alt funct 
	GPIO_PORTF_DR8R_R 	|= 	 ((1<<4)|(1<<3)|(1<<2)|(1<<1)|(1<<0));    // can drive up to 8mA out
	GPIO_PORTF_DEN_R 		|= 	 ((1<<4)|(1<<3)|(1<<2)|(1<<1)|(1<<0));     // enable digital I/O 
  GPIO_PORTF_AMSEL_R 	&= 	~((1<<4)|(1<<3)|(1<<2)|(1<<1)|(1<<0));     // no analog 
	GPIO_PORTF_DATA_R 	&= 	~((1<<4)|(1<<3)|(1<<2)|(1<<1)|(1<<0)); 		//set to zero
	
	
	//////////////////////////////////////////////////
	//Enable systick interrupts for writing to LCD 	//
	//////////////////////////////////////////////////
  uint32_t sysTickReloadCount = (80000000/LCD_REFRESH_HZ/SCREEN_H);	
	NVIC_ST_CTRL_R = 0;           // disable SysTick during setup
	NVIC_ST_RELOAD_R = sysTickReloadCount-1;     // reload value (1/60)/(1/80e6)=571428
  NVIC_ST_CURRENT_R = 0;        // any write to current clears it
  NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x20000000; // priority 1     
	NVIC_ST_CTRL_R = 0x07;  // enable with core clock and interrupts	
	


	writeBuffer = ScreenBufferA;
	readBuffer = ScreenBufferB;
}

uint8_t * Screen_GetBuffer(void){
	return writeBuffer;
}

void Screen_SetBufferIsFull(void){
	writeBufferIsFull = true;
	writeBufferAvailable = false;
}


bool Screen_IsBufferAvailable(void){
	return writeBufferAvailable;
}

void Screen_ClearBuffer(void){ //780us
	uint16_t i;
	for(i=0;i<BUFFER_SIZE;i++){
		writeBuffer[i]=0x00; //zeros correspond to the default clear pixel state
	}
}


/*************************************/
/********* Private Functions *********/
/*************************************/

void SysTick_Handler(void){
	static const uint8_t minLcdUpdatesPerFrame = LCD_REFRESH_HZ/FRAME_REFRESH_HZ;
  static uint8_t rowIndex = 0;
	static uint8_t lcdUpdates = 0;
	
	//PE1 = (1<<1); //debug
	PrintNextRow(rowIndex);
	rowIndex = (rowIndex+1)%SCREEN_H;
	
	if(rowIndex==0 and ++lcdUpdates>=minLcdUpdatesPerFrame and writeBufferIsFull)
	{
			SwitchBuffer();
			writeBufferAvailable = true;
			lcdUpdates = 0;
			writeBufferIsFull = false;
	}
	//PE1 = 0x00; //debug
}


static void SwitchBuffer(void){
	if(writeBuffer == ScreenBufferA){
		writeBuffer = ScreenBufferB;
		readBuffer = ScreenBufferA;
	}
	else{
		writeBuffer = ScreenBufferA;
		readBuffer = ScreenBufferB;
	} 
}



//Data Clock: DataClk_PF1
//static void DataClock(void){
//	unsigned long i=17; //specific number doesn't matter
//	DataClk_PF1 = (1<<1);
//	i=i/i; //Delay enough to keep peak at least ~80ns wide
//	DataClk_PF1 =	0x00;
//}

//Latch Clock: LatchClk_PF2
static void LatchClock(void){
	unsigned long i=17; //specific number doesn't matter
	LatchClk_PF2 = (1<<2);
	i=i/i; //Delay enough to keep peak at least ~80ns wide
	LatchClk_PF2 =	0x00;
}

//DIO1: FrameClk_PF3
static void FrameClock(void){
	FrameClk_PF3 =	 (1<<3);
	LatchClock();
	FrameClk_PF3 =	0x00;
}



////SDI_1(left): PF4 --- SDI_2(right): PF0
//void SetPixels(uint8_t left, uint8_t right){
//	SerialData_PF04 = ((right)<<0)|((left)<<4);
//	DataClock();
//}


static void delayns(void){
	unsigned int i;
	i=i/i/i;
}


static void PrintNextRow(uint8_t rowIndex){
	uint8_t bytesPerRow,colIndex;
	uint8_t bufferByteLeft, bufferByteRight,bitValLeft,bitValRight;
	bytesPerRow = SCREEN_W>>3;// divided by 8 (bytes)
	
	
	
		for(colIndex=0;colIndex<(bytesPerRow>>1);++colIndex){
			bufferByteLeft = readBuffer[rowIndex*bytesPerRow+colIndex];
			bufferByteRight = readBuffer[rowIndex*bytesPerRow+colIndex+(bytesPerRow>>1)];

			DataClk_PF1 = (1<<1);
			bitValLeft = (bufferByteLeft&(1<<7))>>7;
			bitValRight = (bufferByteRight&(1<<7))>>7;
			SerialData_PF04 = ((bitValRight)<<0)|((bitValLeft)<<4);
			DataClk_PF1 =	0x00; //Clocks in data on falling clock edge
			delayns();
			
			DataClk_PF1 = (1<<1);
			bitValLeft = (bufferByteLeft&(1<<6))>>6;
			bitValRight = (bufferByteRight&(1<<6))>>6;
			SerialData_PF04 = ((bitValRight)<<0)|((bitValLeft)<<4);
			DataClk_PF1 =	0x00; //Clocks in data on falling clock edge
			delayns();
			
			DataClk_PF1 = (1<<1);
			bitValLeft = (bufferByteLeft&(1<<5))>>5;
			bitValRight = (bufferByteRight&(1<<5))>>5;
			SerialData_PF04 = ((bitValRight)<<0)|((bitValLeft)<<4);
			DataClk_PF1 =	0x00; //Clocks in data on falling clock edge
			delayns();
			
			DataClk_PF1 = (1<<1);
			bitValLeft = (bufferByteLeft&(1<<4))>>4;
			bitValRight = (bufferByteRight&(1<<4))>>4;
			SerialData_PF04 = ((bitValRight)<<0)|((bitValLeft)<<4);
			DataClk_PF1 =	0x00; //Clocks in data on falling clock edge
			delayns();
			
			DataClk_PF1 = (1<<1);
			bitValLeft = (bufferByteLeft&(1<<3))>>3;
			bitValRight = (bufferByteRight&(1<<3))>>3;
			SerialData_PF04 = ((bitValRight)<<0)|((bitValLeft)<<4);
			DataClk_PF1 =	0x00; //Clocks in data on falling clock edge
			delayns();
			
			DataClk_PF1 = (1<<1);
			bitValLeft = (bufferByteLeft&(1<<2))>>2;
			bitValRight = (bufferByteRight&(1<<2))>>2;
			SerialData_PF04 = ((bitValRight)<<0)|((bitValLeft)<<4);
			DataClk_PF1 =	0x00; //Clocks in data on falling clock edge
			delayns();
			
			DataClk_PF1 = (1<<1);
			bitValLeft = (bufferByteLeft&(1<<1))>>1;
			bitValRight = (bufferByteRight&(1<<1))>>1;
			SerialData_PF04 = ((bitValRight)<<0)|((bitValLeft)<<4);
			DataClk_PF1 =	0x00; //Clocks in data on falling clock edge
			delayns();
			
			DataClk_PF1 = (1<<1);
			bitValLeft = (bufferByteLeft&(1<<0))>>0;
			bitValRight = (bufferByteRight&(1<<0))>>0;
			SerialData_PF04 = ((bitValRight)<<0)|((bitValLeft)<<4);
			DataClk_PF1 =	0x00; //Clocks in data on falling clock edge
		}
		if(rowIndex==0)
			FrameClock();
		else
			LatchClock();
}



