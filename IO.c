
#include "tm4c123gh6pm.h"
#include "screen.h"
#include "IO.h"
#include "fonts.h"


typedef struct
{
	bool enabled;
	uint8_t delta;
	uint32_t phaseAccum;
	SoundEffect_t *sound;
	
} SoundInfo_t;


const uint8_t maxSounds = 3;
uint8_t soundSlot = 0;
SoundInfo_t soundsList[maxSounds];
bool mute = false;
const size_t strBuffLen = 6;
char stringBuffer[strBuffLen] = {0};

char * DecToStr(unsigned num, char *str);
void LoadFrameBuffer( uint16_t xpos, 
											uint16_t ypos, 
											const uint8_t *spriteptr, 
											uint16_t width, 
											uint16_t height);

void IO_Init(void){
	unsigned long volatile delay;
	uint8_t i;
	
	/////////////////////////////////////////
	// Prepare the LCD					            //
	//////////////////////////////////////////
	
	Screen_Init();
	
	//////////////////////////////////////////
	// Enable port A for buttons            //
	//////////////////////////////////////////
	
	if((SYSCTL_RCGC2_R & (1<<0)) == 0) 								//If not yet active
	{
		SYSCTL_RCGC2_R |= (1<<0); 											// activate port A clock
		while((SYSCTL_PRGPIO_R & (1<<0)) == 0){};				// Wait for clock to stabalize 
	}
  GPIO_PORTA_CR_R 		|=  ((1<<4)|(1<<3)|(1<<2));   // allow changes to PA4:2
  GPIO_PORTA_DIR_R 		&= ~((1<<4)|(1<<3)|(1<<2));   // make inputs
	GPIO_PORTA_AFSEL_R 	&= ~((1<<4)|(1<<3)|(1<<2));  	// disable alt funct 
	GPIO_PORTA_DEN_R 		|=  ((1<<4)|(1<<3)|(1<<2));   // enable digital I/O 
  GPIO_PORTA_AMSEL_R 	&= ~((1<<4)|(1<<3)|(1<<2));   // no analog 
	
	//////////////////////////////////////////
	// Enable ADC to read in Pots           //
	//////////////////////////////////////////
	
	if((SYSCTL_RCGC2_R & (1<<4)) == 0)						// If not yet active
	{ 				
		SYSCTL_RCGC2_R |= (1<<4); 									// activate port E clock
		while((SYSCTL_PRGPIO_R & (1<<4)) == 0){};		// Wait for clock to stabalize 
	}
	
  GPIO_PORTE_DIR_R 		&= 	~((1<<3)|(1<<2));   // make PE3,2 input
  GPIO_PORTE_AFSEL_R 	|= 	 ((1<<3)|(1<<2));   // enable alternate function
  GPIO_PORTE_DEN_R 		&= 	~((1<<3)|(1<<2));   // disable digital I/O
  GPIO_PORTE_AMSEL_R 	|= 	 ((1<<3)|(1<<2));   // enable analog function
	
  SYSCTL_RCGC0_R 			|= 	 0x00010000;   			// activate ADC0
  delay |= SYSCTL_RCGC2_R;        
  SYSCTL_RCGC0_R 			&= 	~0x00000300;  			// configure ADC0 for 125K
	ADC0_SAC_R 					|=	 0x06;							// Use x64 hardware averaging
  /////ADC0_SSPRI_R 				= 	 0x0123;          	// Sequencer 3 is highest priority
  ADC0_ACTSS_R 				&= 	~0x01;	        		// disable sample sequencer 0
  ADC0_EMUX_R 				&= 	~0x0F;   	      		// seq0 processor triggered (set ADCPSSIn bit)
	ADC0_SSMUX0_R 			&= 	~0x0FFF;
  ADC0_SSMUX0_R 			|= 	 0x10;             	// First sample: AIN0(PE3), 2nd sample: AIN1 (PE2)
  ADC0_SSCTL0_R 			|= 	 0x60;         			// no temp, no diff. 2nd sample is end and has INT
  ADC0_ACTSS_R 				|= 	 0x01;         			// enable sample sequencer 0
	

	//////////////////////////////////////////
	// Enable SPI for DAC IC                //
	//////////////////////////////////////////
	
	//Clock prescale: SSInClk = SysClk / (CPSDVSR*(1+SCR))
	//DAC has a max clk freq of 20 MHz
	//SysClk = 80 Mhz
	//CPSDVSR: Even number from 2 to 254
	//SCR: Any value from 0 to 255
	//A SPI freq of 1 MHz should be plenty. 1 MHz = 80 MHz / (80*(1+0))
	
	//Using Freescale SPI with polarity and phase = 0
	
	SYSCTL_RCGCSSI_R		|= (1<<2);					//Enable SSI Module 2
	if((SYSCTL_RCGC2_R & (1<<1)) == 0){ 		//If not yet active
		SYSCTL_RCGC2_R |= (1<<1); 						// 1) activate port B clock
		while((SYSCTL_PRGPIO_R & (1<<1)) == 0){}; //Wait till port is ready
		//delay = SYSCTL_RCGC2_R;								// Give clock time to stabalize 
	}
	
	//GPIO_PORTB_DIR_R 		|= 	 ((1<<7)|(1<<5)|(1<<4));   // make PB7,5,4 outputs
  GPIO_PORTB_AFSEL_R 	|= 	 ((1<<7)|(1<<5)|(1<<4));   // enable alternate function
  GPIO_PORTB_DEN_R 		|= 	 ((1<<7)|(1<<5)|(1<<4));   // disable digital I/O
	GPIO_PORTB_AMSEL_R = 0;

	GPIO_PORTB_PCTL_R		|=   0x20220000;			//Set alternate peripheral to SSI
	SSI2_CR1_R			=0;//		&=	~0x02;						//Disable SSI2
	SSI2_CR1_R					&=	~0x04;						//Set as master
	SSI2_CC_R						&=	~0xF;							//Use system clock as clock source
	SSI2_CPSR_R					|=	 0x50;						//Set CPSDVSR	= 80;
	SSI2_CR0_R					&=	~0xFF00;					//Set SCR = 0
	SSI2_CR0_R					&=	~((1<<7)|(1<<6));	//Set phase and polarity to zero
	SSI2_CR0_R					&=	~((1<<5)|(1<<4));	//Set SSE format to Freescale SPI
	SSI2_CR0_R					|=	 0xF;							//Set datasize to 16 bits
	SSI2_CR1_R					|=	 0x02;						//Enable SSI2
	
	///////////////////////////////////////////
	// Enable timer and interrupts for audio //
	///////////////////////////////////////////
	
  SYSCTL_RCGCTIMER_R |= 0x01;   // 0) activate TIMER0
  //PeriodicTask = task;          // user function
  TIMER0_CTL_R = 0x00000000;    // 1) disable TIMER0A during setup
  TIMER0_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER0_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER0_TAILR_R = 5000-1;    // 4) reload value
  TIMER0_TAPR_R = 0;            // 5) bus clock resolution
  TIMER0_ICR_R = 0x00000001;    // 6) clear TIMER0A timeout flag
  TIMER0_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF); // 8) priority 0
// interrupts enabled in the main program after all devices initialized
// vector number 35, interrupt number 19
  NVIC_EN0_R = 1<<19;           // 9) enable IRQ 19 in NVIC
  TIMER0_CTL_R = 0x00000001;    // 10) enable TIMER0A
	

	for(i=0;i<maxSounds;++i){
		soundsList[i].enabled = false;
	}
	
}

void TIMER0A_Handler(void){
	uint16_t amplitude = 0;
	uint8_t i;
	
  TIMER0_ICR_R = TIMER_ICR_TATOCINT;// acknowledge timer0A timeout
  //(*PeriodicTask)();                // execute user task
	if(!mute){
	
		for(i=0;i<maxSounds;++i){
			
			if(soundsList[i].enabled){
				amplitude += soundsList[i].sound->waveForm[soundsList[i].phaseAccum];
				soundsList[i].phaseAccum += soundsList[i].delta;
				
				if(soundsList[i].phaseAccum >= soundsList[i].sound->len){
					soundsList[i].enabled = false;
				}
			}
		}
		amplitude = amplitude<4095 ? amplitude : 4095;
		
		while((SSI2_SR_R & 0x02)==0){;}
		SSI2_DR_R = (0x3000 | amplitude);
	}
	else{
		for(i=0;i<maxSounds;++i){
			soundsList[i].enabled = false;
		}
	}
}

bool IO_Ready(void){
	if(Screen_IsBufferAvailable()){
		Screen_ClearBuffer();
		return true;
	}
	else return false;
}

void IO_UpdatesCompleted(void){
	Screen_SetBufferIsFull();
}

uint16_t IO_getScreenWidth(void){
	return SCREEN_W;
}
uint16_t IO_getScreenHeight(void){
	return SCREEN_H;
}


void IO_PlaySound(SoundEffect_t * sound){
	soundsList[soundSlot].delta = 1;
	soundsList[soundSlot].phaseAccum = 0;
	soundsList[soundSlot].sound = sound;
	soundsList[soundSlot].enabled = true;
	soundSlot = (soundSlot+1)%3;
}

void IO_ToggleMute(void){
	mute ^= true;
}

void IO_PrintMuteStatus(uint8_t xpos, uint8_t ypos){
	if(mute) IO_PrintString(xpos,ypos,"Muted");
}

bool IO_GetLeftButtonState(void){
	return (GPIO_PORTA_DATA_R & (1<<2))>>2;
}

bool IO_GetCenterButtonState(void){
	return (GPIO_PORTA_DATA_R & (1<<3))>>3;
}

bool IO_GetRightButtonState(void){
	return (GPIO_PORTA_DATA_R & (1<<4))>>4;
}

knobs_t IO_ReadKnobs(void){  
	knobs_t knobs;
  ADC0_PSSI_R = 0x0001;              // 1) initiate SS0
  while((ADC0_RIS_R&0x01)==0){};   // 2) wait for conversion done
	knobs.max = 0xFFF>>2;
	knobs.left = (ADC0_SSFIFO0_R&0xFFF)>>2;
	knobs.right = (ADC0_SSFIFO0_R&0xFFF)>>2;
  ADC0_ISC_R = 0x0001;             // 4) acknowledge completion
  return knobs;
}

void IO_LoadSprite(	const uint16_t xpos, 
										const uint16_t ypos, 
										const Sprite_t spriteData )
{
	uint16_t widthPixels = spriteData.widthBits;
	uint16_t heightPixels = spriteData.heightBits;
	LoadFrameBuffer(xpos,ypos,spriteData.sprite,widthPixels,heightPixels);
}

void IO_PrintNumber(uint16_t xpos, uint8_t ypos, uint16_t num){
	char * str = DecToStr(num,stringBuffer+strBuffLen);
	IO_PrintString(xpos,ypos,str);
}

void IO_PrintString(uint16_t xpos, uint8_t ypos, const char * string) 
{
	uint8_t charToPrint;
	uint8_t infoOffset;
	uint16_t widthPixels;
	uint16_t heightPixels;
	const uint8_t * character;
	uint8_t strPtrOffset = 0;
	
	const uint8_t * fontBitmaps = verdana_9ptBitmaps;
	const FONT_INFO * fontInfo = &verdana_9ptFontInfo;
	const FONT_CHAR_INFO * fontCharInfo = verdana_9ptDescriptors;
	
	
	while(*(string+strPtrOffset)){
		charToPrint = *(string+strPtrOffset);
		++strPtrOffset;
		if(charToPrint == ' '){
			xpos += fontInfo->spacePixels;
			continue;
		}
		infoOffset = charToPrint - fontInfo->startChar;
		widthPixels = fontCharInfo[infoOffset].widthBits;
		heightPixels = fontCharInfo[infoOffset].heightBits;
		character = fontBitmaps + fontCharInfo[infoOffset].offset;
		
		LoadFrameBuffer(xpos,ypos,character,widthPixels,heightPixels);
		
		xpos += (widthPixels+2);
	}

}

char * DecToStr(unsigned num, char *str)
{
    *--str = 0;
    if (!num) *--str = '0';
    for (; num; num/=10) *--str = '0'+num%10;
    return str;
}

void LoadFrameBuffer(uint16_t xpos, uint16_t ypos, const uint8_t *spriteptr, uint16_t width, uint16_t height){
	unsigned short bytesPerRow, row, col;
	unsigned short bufferPixelIndex, bufferByteIndex, bufferBitIndex;
	uint8_t spriteByte, spriteBit, mask;
	uint8_t * writeBufferPtr = Screen_GetBuffer();
	//Error checking
	if(	width<=0 || 								//invalid value
			height<=0 || 								//invalid value
			(xpos+width) > SCREEN_W || 	//Off right of screen
			(ypos+height) > SCREEN_H)		//Off bottom of screen
	{return;}
	// Determine how many bytes each row occupies
	// All rows must be right padded to the nearest byte
	bytesPerRow = width>>3;
	if(width%8 > 0){
		bytesPerRow = bytesPerRow + 1;
	}
	for(row=0;row<height;row++){
		for(col=0;col<width;col++){
			bufferPixelIndex = SCREEN_W*(ypos+row)+(xpos+col);
			bufferByteIndex = bufferPixelIndex>>3;
			bufferBitIndex = bufferPixelIndex-((bufferPixelIndex>>3)<<3); //fast bufferPixelIndex%8
			spriteByte = spriteptr[(col>>3) + row*bytesPerRow];
			spriteBit = (spriteByte << (col%8)) & 0x80;
			if(spriteBit==0){
			    mask = (spriteBit|0x7F)>>bufferBitIndex;
			    mask = mask | (0xFF<<(8-bufferBitIndex));
			    writeBufferPtr[bufferByteIndex] &= mask;
			}
			else{
			    mask = (spriteBit >> bufferBitIndex);
			    writeBufferPtr[bufferByteIndex] |= mask;
			}
		}
	}
}


