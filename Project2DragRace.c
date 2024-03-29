// Course number: CECS 346
// Assignment number: Project 2 Drag Race
// Authors: Roy Mears, Ivan Martinez, James Henry
// Date: 3/29/2024
// System Requirements:
// A system based on a drag race. A ‘Christmas Tree’ is the light signal that drivers use to
// determine when to accelerate (ie start the race), which is simulated by our LEDs. A track has
// multiple sensors to determine if a car is in the proper position (“staged”), simulated by pressing
// our “lane sensor” buttons, one per lane (left lane and right lane). Our project will determine the
// winner based on who releases their button fastest after the green LEDs are lit. A reset button will
// be used to override the state machine current state and reset the board back to the initial state
// (similar to the reset button on the LaunchPad). We use PLL to set the system clock to be 25MHz.

// Documentation Section
// main.c
// Runs on TM4C123
// Project2DragRace
// Authors: Roy Mears, Ivan Martinez, James Henry
// Date: 3/29/2024
// DraGraceStarter.c 
// Starter file for CECS346 Project 2
/*
Hardware Design
	PortA controls sensors 
		left sensor 	- PA2
		right sensor 	- PA3
		
	PortB LEDs 
		rightRed 		- PB0
		leftRed 		- PB1
		rightGreen 	- PB2
		leftGreen 	- PB3
	
 	PortC LEDs
		rightYellow1 - PC4
		leftYellow1  - PC5
		rightYellow2 - PC6
		rightYellow2 - PC7
		
 	PortE is for reset 
		reset - PE0
*/




#include "tm4c123gh6pm.h"
#include <stdint.h> // C99 data types
#include <stdbool.h> // provides boolean data type
#include "PLL.h"
#include "SysTick.h"
#include "Sensors_Lights.h"

// Function Prototypes (external functions from startup.s)
extern void DisableInterrupts(void); // Disable interrupts
extern void EnableInterrupts(void);  // Enable interrupts
extern void WaitForInterrupt(void);  // go to low power mode while waiting for the next interrupt

// Function Prototypes
void Sensors_Init(void); // Initialize the left and right push buttons
void Reset_Init(void);   // Initialize reset button
void LightsB_Init(void);  // Initialize portB race lights 
void LightsC_Init(void);  // Initialize portC race lights 
void SysTick_Init(void); // Initialize SysTick timer with interrupt enabled 
void SysTick_Start(uint32_t reload); // Start systick timer with specified reload value.
void System_Init(void);

// Bit addresses for two sensors
#define PUSHBUTTONS_PORTA 						(*((volatile unsigned long *)0x40004030))//PA2, PA3
// Bit addresses for 8 color lights
#define LIGHT_PORTB					 					(*((volatile unsigned long *)0x4000503C))//PB0, PB1, PB2, PB3
#define LIGHT_PORTC										(*((volatile unsigned long *)0x400063C0))//PC4, PC5, PC6, PC7
// Bit addresses for reset button
#define RESET 												(*((volatile unsigned long *)0x40024004))//reset - PE0

// FSM  Struct                                                                           
struct State { 
	uint8_t Out;
	uint8_t Time;     // multiple of 0.5 second
	uint8_t Next[4];
};

typedef const struct State STyp;

// assign a value to all states in Drag Race FSM
enum DraGrace_states {
init, 
wait, 
yellow1, 
yellow2, 
go, 
falseStartLeft, 
falseStartRight, 
falseStartBoth, 
winLeft, 
winRight, 
bothWin 
};


// Drag Race FSM
// Out  Time	Next States
STyp DraGrace_FSM[11] = { // input 11, 10, 01, 00
	{0xFF, 2, {wait, wait, wait, wait}},	  																	 // initialize state, all LEDs on, SysTick Timer restarts      		
	{0x00, 1, {wait, wait, wait, yellow1}},																		 // waiting state, all LEDs off									
	{0xC0, 1, {falseStartBoth, falseStartLeft, falseStartRight, yellow2}},		 // countdown yellow1 state, yellow1L & yellow1R LEDs on																			
	{0x30, 1, {falseStartBoth, falseStartLeft, falseStartRight, go}},			     // countdown yellow2 state, yellow2L & yellow2R LEDs on
	{0x0C, 1, {bothWin, winLeft, winRight, go}},													     // go state, GreenLeft & Gr LEDs on
	{0x02, 2, {wait, wait, wait, wait}},																	     // false start left, redLeft LED on
	{0x01, 2, {wait, wait, wait, wait}},																	     // false start right, redRight LED on
	{0x03, 2, {wait, wait, wait, wait}},																	     // false start both, redleft and redRight LEDs on
	{0x08, 2, {wait, wait, wait, wait}},																	     // win left, GreenLeft LED on
	{0x04, 2, {wait, wait, wait, wait}},																	     // win right, GreenRight LED on
	{0x0C, 2, {wait, wait, wait, wait}}																		     // win both, GreenLeft & Gr LED on
	};

uint8_t S;  // current state index
uint8_t timesup; // global variable flag for SysTick handler
uint8_t Input;
uint8_t reset;  // flag to reset the system, set by the reset button located at breadboard, not the launchpad reset button.
	
	
int main(void){
	System_Init();
		
  while(1){
		while (!reset) {
			  LIGHT_PORTB = (DraGrace_FSM[S].Out & LIGHT_PORTB_MASK); // was LIGHT_PORTB = (DraGrace_FSM[S].Out);
			  LIGHT_PORTC = (DraGrace_FSM[S].Out & LIGHT_PORTC_MASK); // was LIGHT_PORTC = (DraGrace_FSM[S].Out);
				SysTick_Start(DraGrace_FSM[S].Time * HALF_SEC);  // start systick with defined period
			
			while((!timesup)&&(!reset)){
			  WaitForInterrupt();
			}
			timesup=false;
			S = DraGrace_FSM[S].Next[Input];
			SysTick_Stop();
		}
		reset = false;
		S = init;
  }
}

// Initializes pll, all sensors, all lights, reset button, systick timer and initialize constants.
// No parameters are passed to this function. It does not return anything.
// Dependencies: Requires access to Port A,B,C,and E registers.
void System_Init(void) {
	DisableInterrupts();  
	PLL_Init();
	Sensors_Init(); 
	LightsB_Init();
	LightsC_Init();
	Reset_Init(); 
	SysTick_Init();
	S = init; // initial state set
	timesup = false; 
	reset = false;
  Input = PUSHBUTTONS_PORTA >> 2;		// left shift pushbuttons by two for input
	EnableInterrupts();
}
	

// ISR that Handles GPIO Port A interrupts. 
// When Port A interrupt triggers, poll buttons and take in input from sensors.
// No parameters are passed to this function. It does not return anything.
void GPIOPortA_Handler(void){
	unsigned long i;
	for (i = 0; i < 182828; i++) {}
	if ((GPIO_PORTA_RIS_R & PUSHBUTTON_LEFT_MASK)||(GPIO_PORTA_RIS_R & PUSHBUTTON_RIGHT_MASK)){ // if left or right or both button is pressed
					GPIO_PORTA_ICR_R = PUSHBUTTONS_MASK;  // clear flag
					Input = PUSHBUTTONS_PORTA >> 2;		//take 
	}
}

// ISR that Handles GPIO Port E interrupts (reset). 
// When Port E interrupt triggers, do what's necessary then clear the flag and sets global variable reset to true.
// This Interrupt Service Routine (ISR) handles the interrupts generated by Port E.
// No parameters are passed to this function. It does not return anything.
// Resets the board to the initalization state: 
void GPIOPortE_Handler(void) {
	unsigned long i;
	for (i = 0; i < 182828; i++) {}
	GPIO_PORTE_ICR_R 	|= RESET_MASK;	// clear flag	
	if(RESET & RESET_MASK){
	reset = true;
	}		
}

// ISR that Handles SysTick generated interrupts. 
// When timer interrupt triggers, set timesup to true
// This Interrupt Service Routine (ISR) handles the interrupts generated by the SysTick timer.
// No parameters are passed to this function. It does not return anything.
void SysTick_Handler(void) {
	timesup = true;
}


