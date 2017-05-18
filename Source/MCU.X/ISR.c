#include <p18f25k22.h>
#include <timers.h>
#include <usart.h>

#include "ISR.h"

#define SERVO_LEFT_ON_LO_COMP       0b10111000      // 2.4 ms
#define SERVO_LEFT_ON_HI_COMP       0b00001011
#define SERVO_LEFT_OFF_LO_COMP      0b11110000      // 17.6 ms
#define SERVO_LEFT_OFF_HI_COMP      0b01010101

#define SERVO_RIGHT_ON_LO_COMP      0b11101110      // 0.6 ms
#define SERVO_RIGHT_ON_HI_COMP      0b00000010
#define SERVO_RIGHT_OFF_LO_COMP     0b10111010      // 19.4 ms
#define SERVO_RIGHT_OFF_HI_COMP     0b01011110

#define SERVO_MID_ON_LO_COMP        0b11010110      // 1.4 ms
#define SERVO_MID_ON_HI_COMP        0b00000110
#define SERVO_MID_OFF_LO_COMP       0b11010010      // 18.6 ms
#define SERVO_MID_OFF_HI_COMP       0b01011010

#define RISING_EDGE                 0
#define FALLING_EDGE                1

#define POINT_FORWARD               0
#define POINT_LEFT                  1
#define POINT_RIGHT                 2

char addr;


// FUNCTION PROTOTYPES
void intHandler( void );
//void lowPriorityINT( void );

/*************************** INTERRUPT HANDLERS *******************************/

#pragma code isr = 0x08
void isr()
{
    _asm GOTO intHandler _endasm     // Branch to interrupt function
}
#pragma code

#pragma interrupt intHandler
void intHandler()
{
    // LEFT SENSOR INTERRUPT SERVICE
    if (PIR1bits.CCP1IF == 1)               // Capture flag set event
    {
        if (edge == RISING_EDGE)                      // If rising edge
        {
            timeRise = 0;
            WriteTimer1(0);
            CCP1CONbits.CCP1M0 = 0;         // CCP1M = 0100 -> switch to falling edge capture
            CCP1CONbits.CCP1M1 = 0;
            CCP1CONbits.CCP1M2 = 1;
            CCP1CONbits.CCP1M3 = 0;
            edge = FALLING_EDGE;
        }
        else if (edge == FALLING_EDGE)
        {
            timeFall = ReadTimer1();
            duration = timeFall - timeRise;
            newDistance = 1;
            CCP1CONbits.CCP1M0 = 1;         // CCP1M = 0101 -> switch to rising edge capture
            CCP1CONbits.CCP1M1 = 0;
            CCP1CONbits.CCP1M2 = 1;
            CCP1CONbits.CCP1M3 = 0;
            edge = RISING_EDGE;
        }
        PIR1bits.CCP1IF = 0;                // Clear Interrupt flag
    }

    // SERVO CONTROL
    if ( PIR2bits.CCP2IF )
    {
        if ( servoEdge == RISING_EDGE )
        {
            LATAbits.LATA0 = 1;
            if ( servoDirection == POINT_FORWARD )
            {
                CCPR2H = SERVO_MID_ON_HI_COMP;
                CCPR2L = SERVO_MID_ON_LO_COMP;
            }
            else if ( servoDirection == POINT_LEFT )
            {
                CCPR2H = SERVO_LEFT_ON_HI_COMP;
                CCPR2L = SERVO_LEFT_ON_LO_COMP;
            }
            else if ( servoDirection == POINT_RIGHT )
            {
                CCPR2H = SERVO_RIGHT_ON_HI_COMP;
                CCPR2L = SERVO_RIGHT_ON_LO_COMP;
            }

            servoEdge = FALLING_EDGE;
            WriteTimer3(0);
        }

        else if (servoEdge = FALLING_EDGE)
        {
            LATAbits.LATA0 = 0;
            if ( servoDirection == POINT_FORWARD )
            {
                CCPR2H = SERVO_MID_OFF_HI_COMP;
                CCPR2L = SERVO_MID_OFF_LO_COMP;
            }
            else if ( servoDirection == POINT_LEFT )
            {
                CCPR2H = SERVO_LEFT_OFF_HI_COMP;
                CCPR2L = SERVO_LEFT_OFF_LO_COMP;
            }
            else if ( servoDirection == POINT_RIGHT )
            {
                CCPR2H = SERVO_RIGHT_OFF_HI_COMP;
                CCPR2L = SERVO_RIGHT_OFF_LO_COMP;
            }

            servoEdge = RISING_EDGE;
            WriteTimer3(0);
        }
        PIR2bits.CCP2IF = 0;
    }

    // BLUETOOTH COMMS
    else if ( PIR1bits.RC1IF )
    {
        frombluetooth = Read1USART();
        Write1USART(frombluetooth);
        newBluetoothCmd = 1;
        PIR1bits.RC1IF = 0;
    }

    // TIMER0
    else if ( INTCONbits.TMR0IF )
    {
        timer0Count++;
        INTCONbits.TMR0IF = 0;
    }

    // I2C
    else if (PIR1bits.SSP1IF)                                // SSP interrupt
    {
        if (SSP1STATbits.BF)
        {
            if (!SSP1STATbits.R_NOT_W)                  // Slave reading
            {
                if (SSP1STATbits.D_A == 0)              // address not data
                {
                    addr = SSP1BUF;                     // dummy read to clear BF flag
                    //SSP1STATbits.BF = 0;              // explicitly clear the buffer full flag
                    //PIR1bits.SSP1IF = 0;                // clear interrupt flag
                }
                else if (SSP1STATbits.D_A == 1)         // data
                {
                    userInput = SSP1BUF;
                    newUserInput = 1;
                    //newSpeed = 1;
                    //SSP1STATbits.BF = 0;
                    //PIR1bits.SSP1IF = 0;
                }
            }

            else if (SSP1STATbits.R_NOT_W)              // slave writing
            {
                if (SSP1STATbits.D_A == 0)              // addr
                {
                    addr = SSP1BUF;                     // dummy read to clear BF
                    //SSP1STATbits.BF = 0;              // explicitly clear the buffer full flag
                    SSP1BUF = userInput;
                   // WriteI2C1(speed);
                    //PIR1bits.SSP1IF = 0;                // clear interrupt flag
                    //SSP1CON1bits.CKP = 1;               // release CLK
                }
                else if (SSP1STATbits.D_A == 1)         // data
                {
                    SSP1BUF = userInput;
                    //SSP1STATbits.BF = 0;              // explicitly clear the buffer full flag
                    //PIR1bits.SSP1IF = 0;                // clear interrupt flag
                    //SSP1CON1bits.CKP = 1;               // release CLK
                }
            }
        }
        SSP1CON1bits.CKP = 1;                             // release CLK
        PIR1bits.SSP1IF = 0;
    }
}
#pragma code /* return to the default code section */


/*
#pragma code low_isr = 0x18
void low_isr()
{
    _asm GOTO lowPriorityINT _endasm     // Branch to interrupt function
}
#pragma code

#pragma interrupt lowPriorityINT
void lowPriorityINT()
{
    // Timer interrupt (for servo control)
    if (INTCONbits.TMR0IF)
    {
        pulse_max++;
        pulse_top++;

        if (pulse_max >= MAX_VALUE)
        {
            pulse_max = 0;
            pulse_top = 0;
            LATAbits.LATA0 = 0;
        }

        if (pulse_top == top_value)
        {
            LATAbits.LATA0 = 1;
        }
        WriteTimer0(240);
        INTCONbits.TMR0IF = 0;
    }
}
#pragma code
*/