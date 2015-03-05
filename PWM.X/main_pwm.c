// PIC16F877 Configuration Bit Settings

// 'C' source line config statements

#include <xc.h>

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

// CONFIG
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config CP = OFF         // FLASH Program Memory Code Protection bits (Code protection off)
#pragma config BOREN = OFF      // Brown-out Reset Enable bit (BOR disabled)
#pragma config LVP = OFF        // Low Voltage In-Circuit Serial Programming Enable bit (RB3 is digital I/O, HV on MCLR must be used for programming)
#pragma config CPD = OFF        // Data EE Memory Code Protection (Code Protection off)
#pragma config WRT = OFF        // FLASH Program Memory Write Enable (Unprotected program memory may not be written to by EECON control)
#include <./pic16f877.h>
#define _XTAL_FREQ 20000000
#define DEF 20
unsigned int t0val;
unsigned int pwmcnt = 0,cnt = 0;
unsigned int ad = 50,bs = 50,i;//50~170

void main(void)
{
    ADCON0 = 0b00000000;
    ADCON1 = 0b00010110;
    TRISA = 0x00;
    
    T1CON  = 0b00000111;
    PIR1   = 0b00000001;//1bit flag
    PIE1   = 0b00000001;
    INTCON = 0b11000000;
    //T2CON = 0b00000100;
    //OPTION_REG = 0b01100001;
    //INTCON = 0b11100000;
    t0val = 65411;
    TMR1 = t0val;
    while(1)
    {
        bs = 100;//?????
        ad = 100;
        __delay_ms(1000);
        
    }
}
void interrupt isr(void) //???????
{
    
    if(TMR1IF == 1)
    {
        TMR1IF = 0;            // INT?????????
        TMR1 = t0val;
        PIR1   = 0b00000001;
        if(pwmcnt<bs){
            PORTAbits.RA0 = 1;
        }
        else{
            PORTAbits.RA0 = 0;
        }
        if(pwmcnt<ad){
            PORTAbits.RA1 = 1;
        }
        else{
            PORTAbits.RA1 = 0;
        }
        pwmcnt++;
        if(pwmcnt > 1500){
            pwmcnt = 0;
            cnt++;
        }
        if(cnt>=100)
        {
            if(bs == 50) bs = 170;
            else bs = 50;
            if(ad == 50) ad = 170;
            else ad = 50;
            cnt = 0;
        }
    }
}

/*void main(void)
{
    ADCON0 = 0b00000000;
    ADCON1 = 0b00000111;
    TRISA = 0x00;
    
    //ADCON1 = 0x06;
    //T2CON = 0b00000100;
    //OPTION_REG = 0b01100001;
    //INTCON = 0b11100000;
    //t0val = 65411;
    //TMR0 = t0val;
    while(1)
    {
        PORTAbits.RA0 = 1;
        __delay_ms(1000);
        PORTAbits.RA0 = 0;
        __delay_ms(1000);
    }
}*/
/*void interrupt isr(void) //???????
{
    T0IF = 0;            // INT?????????
    TMR0 = t0val;
    if(pwmcnt<bs){
        PORTAbits.RA0 = 1;
    }
    else{
        PORTAbits.RA0 = 0;
    }
    pwmcnt++;
    if(pwmcnt > 200) pwmcnt = 0;
}*/