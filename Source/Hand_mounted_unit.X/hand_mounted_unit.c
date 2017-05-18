#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <p18f25k22.h>
#include <delays.h>
#include <usart.h>
#include <i2c.h>
#include <pwm.h>

#include "setup.h"

#define     US_PER_TICK .8

// CONFIG BITS
#pragma config WDTEN        = OFF       // Watch Dog Timer Enable OFF
#pragma config LVP          = OFF       // Low Voltage Programming (3.3V) OFF
#pragma config PRICLKEN     = OFF       // Primary Clock OFF
#pragma config IESO         = ON        // Switch to External Clock
#pragma config FOSC         = ECHP      // High speed external OSC
#pragma config XINST        = OFF       // Extended instr set OFF


// GLOBAL VARIABLES

/*
 * LEFT SENSOR UTILITIES
 */
unsigned int edge = 0;                  // 0 is rising edge, 1 is falling edge

unsigned int timeRise = 0;
unsigned int timeFall = 0;
unsigned int duration = 0;

unsigned int newDistance = 0;
unsigned int intensity = 0;
unsigned int distance_cm = 0;


unsigned int newUserInput = 0;
char userInput = '\0';

// FUNCTION PROTOTYPES
void    feedback( int input );
int     calculateDistance( int ticks );
void    trigger( void );
void    sendNewInput( void );

// ISR not yet working ************0


/****************************** MAIN ROUTINE **********************************/
void main() {
    init();

    while(1)
    {
        if ( newDistance == 1 )
        {
            distance_cm = calculateDistance( duration );
            if ( distance_cm >= 100 )
            {
                feedback( 100 );
                //feedback( 100 );
            }

            else if ( distance_cm < 100 )
            {
                feedback( distance_cm );
                //feedback( 100 );
            }
            newDistance = 0;
        }
        else if ( newDistance == 0 )
        {
            trigger();
        }

        if ( newUserInput == 1)
        {
            sendNewInput();
            newUserInput = 0;
         }
    }
}

// duty cycle = input / 200
// or value / 1000

// testing: input from 0 to 60 cm
// 60 = 100% duty
void feedback( int input )
{
// 200 cm = lowest intensity
// 0 cm = highest intensity
    int value = ( 100 - input ) * 10;
    //int value = input * 17;
    CCP3CONbits.DC3B0 = value & 0x01;               //set low bit
    CCP3CONbits.DC3B1 = (value >> 0x01) & 0x01;     //set second lowest
    CCPR3L = (value >> 2);                          //set highest eight
}

// 1 tick = .8uS
// Time (us) = # of ticks * .8us
// Distance in inches = time / 148
// Distance in centimeters = time / 58
int calculateDistance( int ticks )
{
    float time_uS = 0.0f;
    int distance_cm = 0;

    time_uS = ticks * US_PER_TICK;
    distance_cm = (int) (ceil(time_uS / 58));

    return distance_cm;
}

void trigger()
{
    LATAbits.LATA2 = 1;
    Delay10TCYx(7);
    LATAbits.LATA2 = 0;
    Delay1KTCYx(0);
    Delay1KTCYx(0);
}

void sendNewInput()
{
    IdleI2C1();                     // SEN = 1
    StartI2C1();
    IdleI2C1();
    putcI2C1( 0xB0 );       //send address
    IdleI2C1();
    putcI2C1( userInput );  //send data
    IdleI2C1();
    StopI2C1();
}







