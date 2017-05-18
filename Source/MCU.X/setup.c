#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <p18f25k22.h>
#include <delays.h>
#include <timers.h>
#include <i2c.h>
#include <usart.h>

unsigned int spbrg           = 10;  // spbrg = 10 -> brgh = 1 -> baudrate = 113636

// FUNCTION PROTOTYPES
void setupGlobalInterrupts( void );
void setupTimer1( void );
void setupTimer3( void );

void setupTimer0( void );
void setupTimer0_INT( void );

void setupCompare( void );
void setupCompare_INT( void );
void setupServo( void );

void setupTransducer( void );
void setupCapture( void );
void setupCapture_INT( void );
void setupPWM( void );
void setupLeftPWM( void );
void setupRightPWM( void );

void setupBluetooth( void );
void setupBluetooth_INT( void );

void setupI2C_slave( void );
void setupI2C_INT_slave( void );

/*-----------------------------------------------------------
                        INIT ROUTINE
 -----------------------------------------------------------*/
void init()
{
    setupGlobalInterrupts();

    setupTimer1();
    setupTransducer();
    setupCapture();             // CCP module setup
    setupCapture_INT();
    setupPWM();
    setupLeftPWM();
    setupRightPWM();

    setupTimer0();
    setupTimer0_INT();


    setupTimer3();
    setupCompare();
    setupCompare_INT();
    setupServo();

    setupBluetooth();
    setupBluetooth_INT();

    setupI2C_slave();
    setupI2C_INT_slave();

    PIR1 = 0x00;
    PIR2 = 0x00;

    Delay100TCYx(1);          // delay 100 cycles
}

void setupGlobalInterrupts()
{
    RCONbits.IPEN = 0;              // Priority Enable OFF
    INTCONbits.GIE = 1;             // Global Interrupt ON
    INTCONbits.PEIE = 1;            // Peripheral Interrupt Enable
    INTCON2bits.NOT_RBPU = 1;       // PORTB pull up OFF
}

void setupTimer0()
{
    T0CONbits.T08BIT = 0;           // 16-bit timer
    T0CONbits.T0CS = 0;             // instruction cycle clock
    T0CONbits.T0SE = 0;             // low to high edge
    T0CONbits.PSA = 0;              // use prescaler
    T0CONbits.T0PS0 = 0;            // prescale = 110 = 1 : 128. Period per tick = 12.8 us
    T0CONbits.T0PS1 = 1;            // overflow period = 1.68 s. 1 minute = 35.7 overflows
    T0CONbits.T0PS2 = 1;
}

void setupTimer0_INT()
{
    INTCONbits.TMR0IE = 1;
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


void setupBluetooth()
{
    unsigned char USARTConfig = USART_TX_INT_OFF & USART_RX_INT_ON & USART_ASYNCH_MODE
                                & USART_EIGHT_BIT & USART_BRGH_HIGH & USART_CONT_RX;
    
    Close1USART();  // close if USART was open earlier
    Open1USART( USARTConfig, spbrg );

    // TXSTA bit CONFIG
    TXSTA1bits.BRGH = 1;    // High Baud Rate
    TXSTA1bits.TXEN = 1;    // Transmit Enable
    TXSTA1bits.SYNC = 0;    // Asynch
    TXSTA1bits.TX9  = 0;    // 9-bit transmit off

    // RCSTA bit CONFIG
    RCSTA1bits.SPEN = 1;    // Serial Port Enable bit
    RCSTA1bits.CREN = 1;    // Continuous Receive Enable bit
    RCSTA1bits.RX9  = 0;    // 9-bit receive off

    // PIC automatically decides which one is input and which one is output
    ANSELCbits.ANSC6    = 0;  // Set C Digital
    ANSELCbits.ANSC7    = 0;  // Set C Digital
    TRISCbits.RC6       = 1;  // TX pin set as output
    TRISCbits.RC7       = 1;  // RX pin set as input

    Open1USART(USARTConfig, spbrg);
}

void setupBluetooth_INT() {
    PIE1bits.RC1IE  = 1;    // RX1 Interrupt Enable Bit
    //IPR1bits.RC1IP  = 1;    // RX1 Interrupt High priority
}

/*-----------------------------------------------------------
                        SERVO CONTROL
 -----------------------------------------------------------*/
//RA0 controls the servo
void setupServo()
{
    TRISAbits.RA0 = 0;      // RA2 Trigger (Output)
    ANSELAbits.ANSA0 = 0;   // Digital select
    LATAbits.LATA0 = 0;
}

void setupTimer3()
{
    T3CONbits.TMR3CS0 = 0;      // TMR1CS = 01 -> Use instruction clock 20MHz/4, FOSC/4
    T3CONbits.TMR3CS1 = 0;

    T3CONbits.T3CKPS0 = 0;      // T1CKPS = 10 -> 1:4 prescale
    T3CONbits.T3CKPS1 = 1;

    T3CONbits.T3SOSCEN = 0;     // Secondary Oscillator Disabled
    T3CONbits.T3RD16 = 0;       // Enable register read/write in one 16-bit operations. Registers TMR1L and TMR1H
    T3CONbits.TMR3ON = 1;       // Enable TMR1/3/5

    WriteTimer3(0);             // Clear timer3 values
}

// compare uses CCP2
void setupCompare()
{
    CCP2CONbits.CCP2M0 = 0;         // generate software interrupt on compare match
    CCP2CONbits.CCP2M1 = 1;
    CCP2CONbits.CCP2M2 = 0;
    CCP2CONbits.CCP2M3 = 1;

    CCPTMRS0bits.C2TSEL0 = 1;       // Set CCP2 to use TMR3
    CCPTMRS0bits.C2TSEL1 = 0;

    CCPR2H              = 0b01100001;
    CCPR2L              = 0b10101000;   // Values to compare the timer to, initially 0110000110101000 for 25000 ticks = 20ms
}

void setupCompare_INT()
{
    PIE2bits.CCP2IE = 1;        // Enable CCP2 Interrupt
    //IPR2bits.CCP2IP = 1;        // compare high interrupt
}
/*-----------------------------------------------------------
                    SENSOR AND FEEDBACK
 -----------------------------------------------------------*/
void setupTransducer()
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
    //IPR1bits.CCP1IP = 1;        // Transducer capture high interrupt
}

//Setups PWM CCP3
void setupPWM()
{
    // 20MHz
    ANSELBbits.ANSB5 = 0;           // PWM ANSEL
    TRISBbits.RB5 = 1;              // TRIS bit to start the setup, prevent spurious outputs during setup
    //PSTR1CONbits.STR1A = 1;       // Steering enable on port A, probably not needed
    //CCPTMRS0    = 0x00;           // Set CCP1 to use Timer2
    CCPTMRS0bits.C3TSEL0 = 0;       // CCP3 uses timer2
    CCPTMRS0bits.C3TSEL1 = 0;
    PR2         = 0b11111001;       // Set Timer2; Period register; PR2 = 249
    CCP3CON     = 0b00101100;       // Set to PWM mode; 5:4 are 2 LSB bits of duty cycle
    CCPR3L      = 0b00111110;       // Set 8 MSB of duty cycle; Set duty cycle to 0
    T2CON       = 0b00000100;       // Set Postscaler and prescaler to 1 and turn on PWM, TMR2
    CCP3CONbits.DC3B1 = 1;
    CCP3CONbits.DC3B0 = 0;
    TRISBbits.RB5 = 0;              // Set PWM bit for output to enable PWM. Done with setup
}

// CCP4
// Port RB0
void setupLeftPWM()
{
    // 20MHz
    ANSELBbits.ANSB0 = 0;           // PWM ANSEL
    TRISBbits.RB0 = 1;              // TRIS bit to start the setup, prevent spurious outputs during setup
    //PSTR1CONbits.STR1A = 1;       // Steering enable on port A, probably not needed
    //CCPTMRS0    = 0x00;           // Set CCP1 to use Timer2
    CCPTMRS1bits.C4TSEL0 = 0;       // CCP2 uses timer2
    CCPTMRS1bits.C4TSEL1 = 0;
    PR2         = 0b11111001;       // Set Timer2; Period register; PR2 = 249
    CCP4CON     = 0b00101100;       // Set to PWM mode; 5:4 are 2 LSB bits of duty cycle
    CCPR4L      = 0b00111110;       // Set 8 MSB of duty cycle; Set duty cycle to 0
    T2CON       = 0b00000100;       // Set Postscaler and prescaler to 1 and turn on PWM, TMR2
    CCP4CONbits.DC4B1 = 1;
    CCP4CONbits.DC4B0 = 0;
    TRISBbits.RB0 = 0;              // Set PWM bit for output to enable PWM. Done with setup
}

// CCP5
void setupRightPWM()
{
    // 20MHz
    //ANSELAbits.ANSA4 = 0;           // PWM ANSEL
    TRISAbits.RA4 = 1;              // TRIS bit to start the setup, prevent spurious outputs during setup
    //PSTR1CONbits.STR1A = 1;       // Steering enable on port A, probably not needed
    //CCPTMRS0    = 0x00;           // Set CCP1 to use Timer2
    CCPTMRS1bits.C5TSEL0 = 0;       // CCP2 uses timer2
    CCPTMRS1bits.C5TSEL1 = 0;
    PR2         = 0b11111001;       // Set Timer2; Period register; PR2 = 249
    CCP5CON     = 0b00101100;       // Set to PWM mode; 5:4 are 2 LSB bits of duty cycle
    CCPR5L      = 0b00111110;       // Set 8 MSB of duty cycle; Set duty cycle to 0
    T2CON       = 0b00000100;       // Set Postscaler and prescaler to 1 and turn on PWM, TMR2
    CCP5CONbits.DC5B1 = 1;
    CCP5CONbits.DC5B0 = 0;
    TRISAbits.RA4 = 0;              // Set PWM bit for output to enable PWM. Done with setup
}

/*-----------------------------------------------------------
                            I2C
 -----------------------------------------------------------*/

//I2C setup for the slave
void setupI2C_slave()
{
    CloseI2C1();

    //TRISCbits.TRISC3    = 1;
    //TRISCbits.TRISC4    = 1;

    ANSELCbits.ANSC3    = 0;  // Set C Digital
    ANSELCbits.ANSC4    = 0;  // Set C Digital
    SSP1CON2bits.SEN    = 0;    // no clock stretching

    OpenI2C1(SLAVE_7, SLEW_OFF);

    SSP1ADD = 0xB0;
}

//I2C interrupt setup for the slave
void setupI2C_INT_slave()
{
    //RCONbits.IPEN   = 1;    // Enable LOW/HIGH Interrupt Priority
    //INTCONbits.GIEH = 1;    // Global Interrupt Enable HIGH
    //INTCONbits.GIEL = 1;    // Global Interrupt Enable LOW
    //INTCONbits.PEIE = 1;    // Peripheral Interrupt Enable
    PIE1bits.SSP1IE = 1;    // SSP1 Interrupt Enable Bit
    //IPR1bits.SSP1IP = 1;    // SSP1 Interrupt Priority HIGH
}