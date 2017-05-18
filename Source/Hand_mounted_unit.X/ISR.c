#include <p18f25k22.h>
#include <timers.h>

#include "ISR.h"

// FUNCTION PROTOTYPES
void ISR1( void );
void distance_ISR( void );

/*************************** INTERRUPT HANDLERS *******************************/

/*-----------------------------------------------------------
                       LEFT SENSOR ROUTINE
 -----------------------------------------------------------*/
#pragma code ISR1 = 0x08
void ISR1()
{
    _asm GOTO distance_ISR _endasm     // Branch to interrupt function
}
#pragma code

#pragma interrupt distance_ISR
void distance_ISR()
{
    // LEFT SENSOR INTERRUPT SERVICE
    if ( PIR1bits.CCP1IF == 1 )               // Capture flag set event
    {
        if (edge == 0)                      // If rising edge
        {
            timeRise = 0;
            WriteTimer1(0);
            CCP1CONbits.CCP1M0 = 0;         // CCP1M = 0100 -> switch to falling edge capture
            CCP1CONbits.CCP1M1 = 0;
            CCP1CONbits.CCP1M2 = 1;
            CCP1CONbits.CCP1M3 = 0;
            edge = 1;
        }
        else if ( edge == 1 )
        {
            timeFall = ReadTimer1();
            duration = timeFall - timeRise;
            newDistance = 1;
            CCP1CONbits.CCP1M0 = 1;         // CCP1M = 0101 -> switch to rising edge capture
            CCP1CONbits.CCP1M1 = 0;
            CCP1CONbits.CCP1M2 = 1;
            CCP1CONbits.CCP1M3 = 0;
            edge = 0;
        }
        PIR1bits.CCP1IF = 0;                // Clear Interrupt flag
    }
    
    // ACK BUTTON
    else if ( INTCON3bits.INT1IF )
    {
        userInput = 'A';
        newUserInput = 1;
        INTCON3bits.INT1IF = 0;
    }

    // PANIC BUTTON
    else if ( INTCON3bits.INT2IF )
    {
        userInput = 'P';
        newUserInput = 1;
        INTCON3bits.INT2IF = 0;
    }
}
#pragma code /* return to the default code section */

