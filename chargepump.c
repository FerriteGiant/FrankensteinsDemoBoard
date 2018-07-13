#include "tm4c123gh6pm.h"
#include "chargepump.h"

void ChargePump_Init(void){
	////////////////////////////////////////////
	//Activate PWM for the charge pump				//
	////////////////////////////////////////////
	SYSCTL_RCC_R				&= ~(SYSCTL_RCC_USEPWMDIV);	//Use undivided system clock as PWM clock

	SYSCTL_RCGC0_R 			|=  (1<<20);								//activate PWM module 0 clock
	if((SYSCTL_RCGC2_R & (1<<1)) == 0)							//If not yet active
	{ 						
		SYSCTL_RCGC2_R |= (1<<1); 										// Activate port B clock
		while((SYSCTL_PRGPIO_R & (1<<1)) == 0){};			// Wait for clock to stabalize 
	}
	GPIO_PORTB_DR4R_R		|=	(1<<6);
	GPIO_PORTB_DEN_R		|=	(1<<6);									//Enable digital output on PB6
	GPIO_PORTB_AFSEL_R	|=  (1<<6);									//Activate Alternate function on PB6
	GPIO_PORTB_PCTL_R		|=  (0x04000000);						//Set alternate peripheral to PWM

	PWM0_0_CTL_R				&= ~(1<<0);									//Disable pwm block until ready to use
	PWM0_0_CTL_R				&= ~(1<<1);									//Set count-down mode
	PWM0_0_GENA_R				|=  (0xC2);									//Drive pwm low on CmpA and high on zero
	PWM0_0_LOAD_R				=		(1000-1);								//80e6/80e3=1000 (80khz pwm)
	PWM0_0_CMPA_R				=		(500-1);								//Set couter value at which CmpA triggers
	PWM0_0_CTL_R				|=  (0x01);									//Enable pwm block
	PWM0_ENABLE_R				|=	(PWM_ENABLE_PWM0EN);		//Enable pwm output to M0PWM0
}
