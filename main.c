#include <stdint.h>
#include <stdbool.h>

#include "tm4c123gh6pm.h"
#include "boids.h"
#include "DisplayTests.h"
#include "IO.h"
#include "Pong.h"
#include "chargepump.h"

bool priorLeftButtonState = 0;
bool priorCenterButtonState = 0;
bool priorRightButtonState = 0;
bool leftButtonPressed = false;
bool centerButtonPressed = false;
bool rightButtonPressed = false;
bool firstRun = true;

enum State
{
	STATE_MAINMENU,
	STATE_PONG,
	STATE_BOIDS,
	STATE_TESTPATTERNS
};


// basic functions defined at end of startup.s
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts


void SystemInit(void){
}

// The #define statement SYSDIV2 initializes
// the PLL to the desired frequency.
#define SYSDIV2 4
// bus frequency is 400MHz/(SYSDIV2+1) = 400MHz/(4+1) = 80 MHz
// configure the system to get its clock from the PLL
void PLL_Init(void){
  // 0) configure the system to use RCC2 for advanced features
  //    such as 400 MHz PLL and non-integer System Clock Divisor
  SYSCTL_RCC2_R |= SYSCTL_RCC2_USERCC2;
  // 1) bypass PLL while initializing
  SYSCTL_RCC2_R |= SYSCTL_RCC2_BYPASS2;
  // 2) select the crystal value and oscillator source
  SYSCTL_RCC_R &= ~SYSCTL_RCC_XTAL_M;   // clear XTAL field
  SYSCTL_RCC_R += SYSCTL_RCC_XTAL_16MHZ;// configure for 16 MHz crystal
  SYSCTL_RCC2_R &= ~SYSCTL_RCC2_OSCSRC2_M;// clear oscillator source field
  SYSCTL_RCC2_R += SYSCTL_RCC2_OSCSRC2_MO;// configure for main oscillator source
  // 3) activate PLL by clearing PWRDN
  SYSCTL_RCC2_R &= ~SYSCTL_RCC2_PWRDN2;
  // 4) set the desired system divider and the system divider least significant bit
  SYSCTL_RCC2_R |= SYSCTL_RCC2_DIV400;  // use 400 MHz PLL
  SYSCTL_RCC2_R = (SYSCTL_RCC2_R&~0x1FC00000) // clear system clock divider field
                  + (SYSDIV2<<22);      // configure for 80 MHz clock
  // 5) wait for the PLL to lock by polling PLLLRIS
  while((SYSCTL_RIS_R&SYSCTL_RIS_PLLLRIS)==0){};
  // 6) enable use of PLL by clearing BYPASS
  SYSCTL_RCC2_R &= ~SYSCTL_RCC2_BYPASS2;
}

//void PORTE_Init(void){
//	if((SYSCTL_RCGC2_R & (1<<4)) == 0){ //If not yet active
//		unsigned long volatile delay;
//		SYSCTL_RCGC2_R |= (1<<4); 			// activate port E clock
//		delay = SYSCTL_RCGC2_R;				// Give clock time to stabalize 
//	}
//  //GPIO_PORTE_CR_R = 0x1F;           // allow changes to PE3-1
//  GPIO_PORTE_DIR_R 		|= 	 ((0<<3)|(0<<2)|(1<<1));     // make PE3,2 input and PE1 output
//	GPIO_PORTE_AFSEL_R 	&= 	~((1<<3)|(1<<2)|(1<<1));  		// disable alt funct 
//	//GPIO_PORTE_DR8R_R 	|= 	 ((1<<3)|(1<<2)|(1<<1));    // can drive up to 8mA out
//	GPIO_PORTE_DEN_R 		|= 	 ((1<<3)|(1<<2)|(1<<1));     // enable digital I/O 
//  GPIO_PORTE_AMSEL_R 	&= 	~((1<<3)|(1<<2)|(1<<1));     // no analog 
//	GPIO_PORTE_DATA_R 	&= 	~((1<<3)|(1<<2)|(1<<1)); 		//set to zero
//}

int main(void){
	
	enum State currentState;
	currentState = STATE_MAINMENU;
	
	DisableInterrupts();
	//Initialize all the things
	PLL_Init();
	ChargePump_Init();
	IO_Init();
	EnableInterrupts();
	

	
//	Pong_Init();
//	Pong_rand();
//	
//	Boids_Init();
	
	
	
	
	
	
	//Down the rabbit hole we go
	while(1){
		
		if(IO_Ready()){
			
		//Left button
		if(IO_GetLeftButtonState() && !priorLeftButtonState)
		{
			priorLeftButtonState = true;
			leftButtonPressed = true;
		}
		else if (!IO_GetLeftButtonState()){
			priorLeftButtonState = false;
			leftButtonPressed = false;
		}
		else if (priorLeftButtonState)
		{
			leftButtonPressed = false;
		}
		
		//Center button
		if(IO_GetCenterButtonState() && !priorCenterButtonState)
		{
			priorCenterButtonState = true;
			centerButtonPressed = true;
		}
		else if (!IO_GetCenterButtonState()){
			priorCenterButtonState = false;
			centerButtonPressed = false;
		}
		else if (priorCenterButtonState)
		{
			centerButtonPressed = false;
		}
		
		//Right button
		if(IO_GetRightButtonState() && !priorRightButtonState)
		{
			priorRightButtonState = true;
			rightButtonPressed = true;
		}
		else if (!IO_GetRightButtonState()){
			priorRightButtonState = false;
			rightButtonPressed = false;
		}
		else if (priorRightButtonState)
		{
			rightButtonPressed = false;
		}
			
		//FSM
		switch(currentState)
			{
				case STATE_MAINMENU:
					IO_PrintString(220,2,"Main Menu");
					IO_PrintString(220,17,"1 Pong");
					IO_PrintString(220,32,"2 Boids");
					IO_PrintString(220,47,"3 Display Tests");
					
					if(leftButtonPressed)
					{
						firstRun = true;
						currentState = STATE_PONG;
					}
					
					else if (centerButtonPressed){
						firstRun = true;
						currentState = STATE_BOIDS;
					}
					
					else if (rightButtonPressed){
						firstRun = true;
						currentState = STATE_TESTPATTERNS;
					}
					
					break;
				
				case STATE_PONG:
					if(leftButtonPressed)
					{
						currentState = STATE_MAINMENU;
						firstRun = true;
					}
//					if(firstRun){
//						IO_PrintString(220,15,"PONG");
//						IO_PrintString(210,45,"Exit");
//						int i = 10000;
//						while(i){
//							--i;
//						}
//					}
					if(firstRun)
					{
						Pong_Init();
						Pong_rand();
						firstRun = false;
					}
				
					Pong_UpdateGame();
					break;
				
				case STATE_BOIDS:
					if(leftButtonPressed)
					{
						currentState = STATE_MAINMENU;
						firstRun = true;
					}
					
					if(firstRun){
						Boids_Init();
						firstRun = false;
					}
					
					Boids_update();
					Boids_load();
					break;
					
				case STATE_TESTPATTERNS:
					if(leftButtonPressed)
					{
						currentState = STATE_MAINMENU;
						firstRun = true;
					}
					DisplayTests_DrawDiag();
					DisplayTests_DrawBorder();
					break;
					
				default:
					break;
			
			}
			
			IO_UpdatesCompleted();
		}
		

			
		
		
//		if(IO_Ready()){
//			
//			IO_PrintMuteStatus(120,45);
//			//DisplayTests_DrawDiag();
//			DisplayTests_DrawBorder();
//			//IO_PrintString(80,10,"PART");
//			//IO_PrintString(83,32,"TWO");

//			//Boids_update(); //9.94ms for 100 boids
//			//Boids_load(); //910us for 100 boids
//		
//			Pong_UpdateGame();
//			
//			IO_UpdatesCompleted();	
//		}
		
//		if(IO_GetLeftButtonState() && !priorLeftButtonState){
//			priorLeftButtonState = 1;
//			IO_ToggleMute();
//		}
//		else if(!IO_GetLeftButtonState()){
//			priorLeftButtonState = 0;
//		}
		
	}
}
