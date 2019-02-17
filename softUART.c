/*
 * File:   SoftUART.c
 * Author: Hammad Tariq
 *
 * Created on February 16, 2019, 5:17 PM
 */


#define _XTAL_FREQ		  20000000 // FOSC = 20MHz

// FCLK = FOSC/4 for PIC18F uC

#define UART0_BAUD		  19200

#define UART0_BIT_PERIOD  (0xFFFF - (_XTAL_FREQ/4)/UART0_BAUD)   // 16bit Timer is used
#define UART0_Rx_Pin 	  PORTBbits.RB0 // pin0 of PORTB is external Interrupt0 of PIC18F uC
#define led0              PORTAbits.RA0
#define led1              PORTAbits.RA1

unsigned char UART0_TMR=0, bit0No=0, UART0_Rx_Byte=0;

void main()
{
    ANSELA=ANSELB=ANSELC=ANSELD=ANSELE=0x00; // disable ADC channels of uC
    TRISA = 0x00; // led0,led1 as outputs
    PORTA=0x00;
	
	// Interrupt Init
	RCONbits.IPEN 		= 0; // set priority if you want
    INTCON2bits.TMR0IP 	= 0;
    INTCONbits.PEIE 	= 0;
    INTCONbits.GIE 		= 0;

	// 1. Enable 'UARTx_Rx_Pin' External Interrupt to -ve EDGE to detect Start bit
    TRISBbits.RB0 		= 1; // SoftUART Rx pin as input
    
    INTCONbits.INT0F 	= 0;
    INTCON2bits.INTEDG0 = 0; // falling edge
    INTCONbits.INT0E 	= 1; // enable External Interrupt
    
    // Init TIMER0 in 16-bit mode, Timer's clock source=Fosc/4, no Pre-scalar
   
    T0CONbits.TMR0ON 	= 0; // Turn-off timer
    T0CONbits.T08BIT 	= 0; // 16bit timer
    T0CONbits.T0CS 		= 0; // System CLOCK as input
    T0CONbits.PSA		= 1; // no Pre-scalar. i.e., clock=Fosc/4
    INTCONbits.TMR0IF	= 0; // clear Interrupt flag if any
    INTCONbits.TMR0IE 	= 1; // enable Timer0 interrupt

    INTCONbits.PEIE 	= 1; 	// enable Peripherial interrupt
    INTCONbits.GIE 		= 1;		// enable Global Interrupt
    
    while (1) // Super loop
    {} // USER code
}

// 3. UARTx_Rx_Pin Interrupt (ROUTINE)
// External Interrupt (UARTx_Rx_Pin) Routine
void interrupt low_priority myISR()
{
    if (INTCONbits.INT0F) // start bit detected
    {
		INTCONbits.INT0F = 0;
        INTCONbits.INT0E=0;
        TMR0 = UART0_BIT_PERIOD/2; // half bit period
        T0CONbits.TMR0ON=1;
    }
    
    if (INTCONbits.TMR0IF)
    {
        INTCONbits.TMR0IF=0;
        
        T0CONbits.TMR0ON=0;
        TMR0 = UART0_BIT_PERIOD; // 1 bit PERIOD
        
        if ( (bit0No==0) && (UART0_Rx_Pin==0) ) // Check start bit (if it is still '0')
        {
          T0CONbits.TMR0ON=1; // Turn on Timer to sample serial data
          bit0No=1;
        }
        else if (bit0No) // Start bit was successfully detected
        {
            if (++bit0No == 10) // 1(Start bit) + 8(data bits) + 1(stop bit)=10bits
            {                        
                if (UART0_Rx_Pin==1) // stop bit detected
                {
                    // UARTx RX Interrupt logic here
                    led0 = ~led0;
                }
                bit0No=0;
                UART0_Rx_Byte=0;
                INTCONbits.INT0F = 0;
                INTCONbits.INT0E=1;
            }
            else
            {
                T0CONbits.TMR0ON=1;

                if (UART0_Rx_Pin)
                    UART0_Rx_Byte =  0x80 | (UART0_Rx_Byte>>1); // lsb first logic
                else
                    UART0_Rx_Byte =  0x00 | (UART0_Rx_Byte>>1);
            }			
        }
    }
}
