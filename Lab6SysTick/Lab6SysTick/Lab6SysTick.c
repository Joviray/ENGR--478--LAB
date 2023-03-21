#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/systick.h"
#include "inc/tm4c123gh6pm.h"

//*****************************************************************************
//
//!	Following code is an example of use of Systick timer, and PWM concept
//! When either button is pressed, var 'count' increaments
//! For odd values of 'count', the leds will be on
//! Higher values of 'count' result to brighter LED intensity
//! 'count' cycles up to 15 then restarts at 0
//
//*****************************************************************************

// global variable visible in Watch window of debugger
// increments at least once per button press
#define RED_MASK 0x02
#define GRN_MASK 0x08
#define SW1				GPIO_PIN_4
#define SW2				GPIO_PIN_0

// 'volatile' for varibles that can be changed by int
volatile int stage = 0x02;
volatile char count = 0; // char is same as uint8_t
volatile uint64_t clock = 0;
volatile uint8_t past_SW1 = 0x11;
volatile uint8_t past_SW2 = 0x11;
volatile uint8_t count_SW1 = 0;
volatile uint8_t count_SW2 = 0;


// only want to react to changes/edges, so need history to compare with

void 
PortFunctionInit(void)
{
	//
	// Enable Peripheral Clocks
	//
	// Needs unlock

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); // User input and output
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); // Debug with AD2, also acts as delay for F's clock to stablize

	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
	HWREG(GPIO_PORTF_BASE + GPIO_O_CR) = 0x1;
	//
	// Enable pin PF4 for GPIOInput
	//
	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_1 | GPIO_PIN_3);

	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, 0x0E); // GPIO_PIN_1-3);
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);

	GPIO_PORTF_PUR_R |= (GPIO_PIN_0 | GPIO_PIN_4);
}

void Systick_Handler(void) // Note this function needs to be added to startup
{						   // cycle 1/32 sec
						   // Normally need to clear int trigger, but not for systick

	clock++;														// Can replicate Arduino millis function if systick period 1ms
	// GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, (clock & 0x10) >> 2); // debug pin2 cycles once per sec
	// uint8_t button = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);
    uint8_t button_SW1 = GPIOPinRead(GPIO_PORTF_BASE, SW1);
	uint8_t button_SW2 = GPIOPinRead(GPIO_PORTF_BASE, SW2);

	if(button_SW1 != past_SW1){
		count_SW1++;
		if (count_SW1 >= 2){
			stage++;
			count_SW1 = 0;

		}
		past_SW1 = button_SW1;
	}
	if(button_SW2 != past_SW2){
		count_SW2++;
		if (count_SW2 >= 2){
			stage--;
			count_SW2 = 0;

		}
		past_SW2 = button_SW2;
	}
	
}



int main(void)
{

	PortFunctionInit();	//initialize the GPIO ports	
    SysTickPeriodSet(SysCtlClockGet()/1000); //1000 Hz systick used for clock
    SysTickEnable(); //starts the counter
	SysTickIntEnable(); //arms the int
	uint8_t light = 0x02; // 64bit unsigned int
	
    while(1){
			if (clock > 2000){ // goes to next stage every 2 seconds
				clock = 0;
				stage++;
			}
			//instead of using this, it would be cool to do a bitshift method
			if(stage < 0){ // stage goes into negative, loops around to stage 3
				stage = 3;
			}
			else if(stage == 0){
				light = 0x02; // red light
			}
			else if(stage == 1){
				light = 0x08; // green light
			}
			else if(stage == 2){
				light = 0x0a; // yellow light 
			}
			else if(stage == 3){
				light = 0x00; // no light 
			}
			else if(stage > 3){
				stage = 0;		// loops back to stage zero
			}
			
			
			//psudo PWM signal, 10% brightness
			if (clock%10 == 0){ // turn on light
				GPIOPinWrite(GPIO_PORTF_BASE, RED_MASK | GRN_MASK, light); 
			}else{ // turn off light
				GPIOPinWrite(GPIO_PORTF_BASE, RED_MASK | GRN_MASK, 0x00); 
			}
    }
	
}