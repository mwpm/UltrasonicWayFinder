#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <p18f25k22.h>
#include <delays.h>
#include <usart.h>
#include <i2c.h>
#include <pwm.h>
#include <PWM.h>

#include "setup.h"

#define US_PER_TICK             .8
#define DISPLAY_THRESHOLD       200
#define WARNING_THRESHOLD       20

#define RISING_EDGE             0
#define FALLING_EDGE            1

#define POINT_FORWARD           0
#define POINT_LEFT              1
#define POINT_RIGHT             2

#define START                   1
#define STOP                    0

/*-----------------------------------------------------------
                        CONFIG BITS
 -----------------------------------------------------------*/
#pragma config WDTEN        = OFF       // Watch Dog Timer Enable OFF
#pragma config LVP          = OFF       // Low Voltage Programming (3.3V) OFF
#pragma config PRICLKEN     = OFF       // Primary Clock OFF
#pragma config IESO         = ON        // Switch to External Clock
#pragma config FOSC         = ECHP      // High speed external OSC
#pragma config XINST        = OFF       // Extended instr set OFF


/*-----------------------------------------------------------
                        GLOBAL VARS
 -----------------------------------------------------------*/

// Left sensor utilities
unsigned int edge = RISING_EDGE;                                          // 0 is rising edge, 1 is falling edge

unsigned int timeRise = 0;
unsigned int timeFall = 0;
unsigned int duration = 0;

unsigned int newDistance = 0;
unsigned int intensity = 0;
unsigned int distanceFront_cm = 0;
unsigned int distanceLeft_cm = 0;
unsigned int distanceRight_cm = 0;

unsigned int scanning = 0;

char dataString[6];                                     // to write to remote monitor

unsigned int newUserInput = 0;
char userInput = 'A';

unsigned int timer0Count = 0;
unsigned int minute = 0;
char minuteString[2];


// Servo control
unsigned int servoEdge = RISING_EDGE;                   // 0 is rising edge
unsigned int servoDirection = POINT_FORWARD;            // 0 = mid, 1 = left, 2 = right

char frombluetooth = 'g';
unsigned int newBluetoothCmd = 0;

char cm[] = " cm\0";

char danger0[] = "Warning!!! Obstacle \0";
char danger1[] = " ahead\0";

char alert[] = "The user is not responding...\nSend help!\0";

char good[] = "Plenty of space...\0";

char help[] = "Help on the way!\0";
//char panic[] = "User has pressed the panic button... Better check on them!\0";


char scanningLeft[] = "Scanning left...\0";
char scanningRight[] = "Scanning right...\0";

char promptLeft[] = "User should turn left...\0";
char promptRight[] = "User should turn right...\0";

/*-----------------------------------------------------------
                    FUNCTION PROTOTYPES
 -----------------------------------------------------------*/
void    feedback( int input );

int     calculateDistance( int ticks );
void    trigger( void );

void    leftFeedback( int startStop );
void    rightFeedback( int startStop );

/*-----------------------------------------------------------
                        MAIN ROUTINE
 -----------------------------------------------------------*/
void main()
{
    init();

    while(1)
    {
        if ( newBluetoothCmd )
        {
            if ( frombluetooth == 'k' )
            {
                Write1USART( '\n' );
                puts1USART( help );
            }
            
            newBluetoothCmd = 0;
        }
        
        if ( timer0Count >= 36 )
        {
            puts1USART( alert );
            Write1USART( '\n' );
            timer0Count = 27;                                    // continuously alert every 15 seconds
        }
        
        if ( newUserInput == 1)
        {
            if ( userInput == 'P')
            {
                puts1USART( alert );
                Write1USART( '\n' );
            }
            newUserInput = 0;
        }


        if ( !newDistance && userInput == 'A')
        {
            timer0Count = 0;                                        // reset alert timer
            leftFeedback( 0 );
            rightFeedback( 0 );
            trigger();
            while ( !newDistance );
        }

        if ( newDistance )
        {
            if ( servoDirection == POINT_FORWARD )                  // Normal operation, scanning distance ahead
            {
                distanceFront_cm = calculateDistance( duration );
                itoa( distanceFront_cm, dataString );

                if ( distanceFront_cm <= DISPLAY_THRESHOLD )        // only display the distance on the remote monitor if less than threshold
                {
                    puts1USART( dataString );                       // display distance measurements on remote monitor
                    puts1USART( cm );
                    Write1USART( '\n' );
                }

                if ( distanceFront_cm <= WARNING_THRESHOLD )        // if the distance in front is less than
                {
                    puts1USART( danger0 );                          // display warning info onto the remote monitor
                    puts1USART( dataString );
                    puts1USART( cm );
                    puts1USART( danger1 );
                    Write1USART( '\n' );
                    puts1USART( scanningLeft );
                    Write1USART( '\n' );

                    servoDirection = POINT_LEFT;                    // change the sensor direction to point left
                    scanning = 1;
                    feedback( 100 );
                    newDistance = 0;
                    Delay10KTCYx(0);                                // delay to complete change of direction
                }

                newDistance = 0;
                // MAY NEED TO RECALCULATE SINCE MOTOR RUNS @ 2-5 V
                if ( distanceFront_cm >= 100 && scanning == 0 )
                {
                    feedback( 100 );
                }

                else if ( distanceFront_cm < 100 && scanning == 0)
                {
                    feedback( distanceFront_cm );
                }
            }

            else if ( servoDirection == POINT_LEFT )                // Scanning: scanning left and right for better path
            {
                distanceLeft_cm = calculateDistance( duration );
                itoa( distanceLeft_cm, dataString );

                if ( distanceLeft_cm > 300 )
                {
                    puts1USART( good );
                }
                else
                {
                    puts1USART( dataString );
                    puts1USART( cm );
                }
                
                Write1USART( '\n' );
                puts1USART( scanningRight );
                Write1USART( '\n' );

                servoDirection = POINT_RIGHT;                       // change the sensor direction to point right
                newDistance = 0;
                Delay10KTCYx(0);                                    // delay to complete change of direction
                Delay10KTCYx(120);
            }

            else if ( servoDirection == POINT_RIGHT )               // Scanning: scanning right
            {
                distanceRight_cm = calculateDistance( duration );
                itoa( distanceRight_cm, dataString );
                if ( distanceRight_cm > 300 )
                {
                    puts1USART( good );
                }
                else
                {
                    puts1USART( dataString );
                    puts1USART( cm );
                }
                Write1USART( '\n' );

                servoDirection = POINT_FORWARD;                     // return the transducer to original pos
                newDistance = 0;
                Delay10KTCYx(0);                                    // delay to complete change of direction

                if ( distanceLeft_cm > distanceRight_cm )
                {
                    leftFeedback( 1 );
                    puts1USART( promptLeft );
                    Write1USART( '\n' );
                }
                else
                {
                    rightFeedback( 1 );
                    puts1USART( promptRight );
                    Write1USART( '\n' );
                }
                scanning = 0;
                userInput = 'S';                                    // stop the functions until the user acknowledge
            }

        }
    }
}


// duty cycle = input / 200
// or value / 1000
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

void leftFeedback( int startStop )
{
    int value;
    if ( startStop == START)
    {
        value = 500;
    }
    else
    {
        value = 0;
    }
    CCP4CONbits.DC4B0 = value & 0x01;               //set low bit
    CCP4CONbits.DC4B1 = (value >> 0x01) & 0x01;     //set second lowest
    CCPR4L = (value >> 2);                          //set highest eight
}

void rightFeedback( int startStop )
{
    int value;
    if ( startStop == START)
    {
        value = 500;
    }
    else
    {
        value = 0;
    }
    
    CCP5CONbits.DC5B0 = value & 0x01;               //set low bit
    CCP5CONbits.DC5B1 = (value >> 0x01) & 0x01;     //set second lowest
    CCPR5L = (value >> 2);                          //set highest eight
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
