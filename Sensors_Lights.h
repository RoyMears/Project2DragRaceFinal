#ifndef SENSORS_LIGHTS
#define SENSORS_LIGHTS
// Sensors_Lights.h
// Starter file for CECS346 Project 2
// By Dr. Min He
#include <stdint.h> // C99 data types

// Initialize the two sensors, enable both edge edge-triggered interrupt for both sensors
void Sensors_Init(void);

// Initialize the reset button
void Reset_Init(void);

// Initialize 8 LEDs
void Lights_Init(void);

// Constant definitions
#define NVIC_EN0_PORTA		  0x00000001  // Bit position for PORTA interrupt in NVIC_EN0_R register. 
#define NVIC_EN0_PORTE		  0x00000010  // Bit position for PORTE interrupt in NVIC_EN0_R register. 
#define NVIC_EN0_PORTF		  0x40000000  // Bit position for PORTF interrupt in NVIC_EN0_R register. 



#define  PUSHBUTTONS_MASK  0x0C
#define  LIGHT_PORTB_MASK  0x0F
#define  LIGHT_PORTC_MASK  0xF0
#define  RESET_MASK  0x01
#define PUSHBUTTON_LEFT_MASK     0x04      
#define PUSHBUTTON_RIGHT_MASK    0x08   

#endif /* SENSORS_LIGHTS */
