#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <p18f25k22.h>
#include <delays.h>
#include <timers.h>
#include <i2c.h>

#define ADDRESS     0x31

// FUNCTION PROTOTYPES
void setupGlobalInterrupts( void );
void setupTimer1( void );

void setupTranducer( void );
void setupCapture( void );
void setupCapture_INT( void );
void setupPWM( void );

void setupAckButton( void );
void setupPanicButton( void );

void setupI2C_master( void );


/****************************** INIT ROUTINE **********************************/
void init()
{
    setupGlobalInterrupts();

    setupTranducer();
    setupTimer1();
    setupCapture();             // CCP module setup
    setupCapture_INT();
    setupPWM();

    setupAckButton();
    setupPanicButton();

    setupI2C_master();

    LATAbits.LATA2 = 0;         // Initial trigger state = 0
    PIR1 = 0x00;

    Delay100TCYx(1);            // delay 100 cycles
}


void setupGlobalInterrupts()
{
    RCONbits.IPEN = 0;          // Priority Enable OFF
    INTCONbits.GIE = 1;         // Global Interrupt ON
    INTCONbits.PEIE = 1;        // Peripheral Interrupt Enable
    INTCON2bits.NOT_RBPU = 1;       // PORTB pull up OFF
}

// TMR1
void setupTimer1()
{
    T1CONbits.TMR1CS0 = 0;      // TMR1CS = 01 -> Use instruction clock 20MHz/4, FOSC/4
    T1CONbits.TMR1CS1 = 0;

    T1CONbits.T1CKPS0 = 0;      // T1CKPS = 10 -> 1:4 prescale
    T1CONbits.T1CKPS1 = 1;

    T1CONbits.T1SOSCEN = 0;     // Secondary Oscillator Disabled
    T1CONbits.T1RD16 = 0;       // Enable register read/write in one 16-bit operations. Registers TMR1L and TMR1H
    T1CONbits.TMR1ON = 1;       // Enable TMR1/3/5

    WriteTimer1(0);             // Clear timer1 values
}

/*-----------------------------------------------------------
                        SENSOR SETUP
 -----------------------------------------------------------*/

void setupTranducer()
{
    TRISAbits.RA2 = 0;      // RA2 Trigger (Output)

    ANSELAbits.ANSA2 = 0;   // Digital select

    LATAbits.LATA2 = 0;
}

// CCP1
void setupCapture()
{
    TRISCbits.TRISC2 = 1;       // CCP1 to input capture
    ANSELCbits.ANSC2 = 0;       // digital select

    CCP1CONbits.CCP1M0 = 1;     // CCP1M = 0101 => Capture mode: every rising edge 0101. Reset to falling edge after interrupt
    CCP1CONbits.CCP1M1 = 0;
    CCP1CONbits.CCP1M2 = 1;
    CCP1CONbits.CCP1M3 = 0;

    CCPTMRS0bits.C1TSEL0 = 0;   // C1TSEL = 00 => capture mode uses Timer1
    CCPTMRS0bits.C1TSEL1 = 0;
}

void setupCapture_INT()
{
    PIE1bits.CCP1IE = 1;        // Enable CCP1 Interrupt
}

//Setups PWM CCP3
void setupPWM()
{
    // 20MHz
    ANSELBbits.ANSB5 = 0;           // PWM ANSEL
    TRISBbits.RB5 = 1;              // TRIS bit to start the setup, prevent spurious outputs during setup
    CCPTMRS0bits.C3TSEL0 = 0;       // Set CCP3 to use TMR2
    CCPTMRS0bits.C3TSEL1 = 0;
    PR2         = 0b11111001;       // Set Timer2; Period register; PR2 = 249
    CCP3CON     = 0b00101100;       // Set to PWM mode; 5:4 are 2 LSB bits of duty cycle
    CCPR3L      = 0b00111110;       // Set 8 MSB of duty cycle; Set duty cycle to 0
    T2CON       = 0b00000100;       // Set Postscaler and prescaler to 1 and turn on PWM, TMR2
    CCP3CONbits.DC3B1 = 1;
    CCP3CONbits.DC3B0 = 0;
    TRISBbits.RB5 = 0;              // Set PWM bit for output to enable PWM. Done with setup
}


void setupI2C_master()
{
    CloseI2C1();        // Close i2c if was opened prior

    //TRISCbits.TRISC3    = 0;
    //TRISCbits.TRISC4    = 0;

    ANSELCbits.ANSC3 = 0;  // Set C Digital  SCL
    ANSELCbits.ANSC4 = 0;  // Set C Digital  SDA

    OpenI2C1( MASTER, SLEW_OFF );
    SSP1ADD = ADDRESS;       // 49 = SSPADD Baud Register used to calculate I2C clock speed (100Khz)
}

// RB1
// INT1
// flag is INTCON3bits.INT1IF
void setupAckButton()
{
    ANSELBbits.ANSB1 = 0;
    TRISBbits.TRISB1 = 1;       // CCP1 to input capture
    INTCON2bits.INTEDG1 = 1;        // rising edge
    INTCON3bits.INT1IE = 1;         // enable INT1
}

// RB2
// INT2
// flag is INTCON3bits.INT2IF
void setupPanicButton()
{
    ANSELBbits.ANSB2 = 0;
    TRISBbits.TRISB2 = 1;
    INTCON2bits.INTEDG2 = 1;
    INTCON3bits.INT2IE = 1;
}