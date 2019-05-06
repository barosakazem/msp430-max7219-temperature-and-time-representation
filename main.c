#include "io430.h"
#include "stdint.h"

char matrix[4][4] ={{'1','2','3','A'},
                    {'4','5','6','B'},
                    {'7','8','9','C'},
                    {'*','0','#','D'}};

// Port1 USCI pins and CS/Load pin
#define SPI_SIMO	BIT2
#define SPI_CLK		BIT4
#define SPI_CS		BIT3	// Load or /CS

// MAX7219 Register addresses
#define MAX_NOOP	0x00
#define MAX_DIGIT0	0x01
#define MAX_DIGIT1	0x02
#define MAX_DIGIT2	0x04
#define MAX_DIGIT3	0x08
#define MAX_DIGIT4	0x10
#define MAX_DIGIT5	0x20
#define MAX_DIGIT6	0x40
#define MAX_DIGIT7	0x80
#define MAX_DECODEMODE	0x09
#define MAX_INTENSITY	0x0A
#define MAX_SCANLIMIT	0x0B
#define MAX_SHUTDOWN	0x0C
#define MAX_DISPLAYTEST	0x0F



// Function prototypes
void spi_init();
void spi_max(unsigned char address, unsigned char data);
void sicaklik_init(void);
void sicaklik_oku(void);
void sicaklik_goster(void);
void saatgoster(void);
void saniyeartir(void);
void keypad(void);
void keycontrol(void);

const uint8_t number[] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};


unsigned int i=0,saniye=0,dakika=0,saat=0;
unsigned int d1,d2,d3,d4,sicaklik_temp,sicaklik_ham;
unsigned int sicaklik=0,sicaklik1=0,sicaklik2=0;
int satir, sutun;
int digit[4];
char tus;
int count=0;

void main(void)
{
	WDTCTL = WDTPW + WDTHOLD; 	// Disable WDT
	DCOCTL = CALDCO_1MHZ; 		// 1 Mhz DCO
	BCSCTL1 = CALBC1_1MHZ;

        TACCR0 = 41000;
        TACCTL0 = CCIE;
        TACTL = MC_1 + ID_3 + TASSEL_2 + TACLR; 
   
        // Setup Port1 pins
          P1DIR |= SPI_SIMO + SPI_CLK + SPI_CS;
          #ifdef USE_MAX7219
          P1OUT |= SPI_CS;		// MAX7219 Chip Select is inactive high
          #endif 
	
        P1DIR|=0xD0;
        P1OUT|=0x01;
        P1REN|=0x01;
        
        P2DIR &=~ 0x0D;
        P2DIR|=0x22;
        P2OUT|=0x0D;
        P2REN|=0x0D;     

       
        sicaklik_init();
	spi_init();			// Init USCI in SPI mode
        
       
        __bis_SR_register(GIE);
	
        
        // Initialise MAX7219
	spi_max(MAX_NOOP, 0x00); 	// NO OP (seems needed after power on)
        spi_max(MAX_SCANLIMIT, 0x07);
	spi_max(MAX_INTENSITY, 0x0F); 	// Display intensity (0x00 to 0x0F)
	spi_max(MAX_DECODEMODE, 0);	
	
         // Clear all rows/digits
	spi_max(MAX_DIGIT0, 0);
	spi_max(MAX_DIGIT1, 0);
	spi_max(MAX_DIGIT2, 0);
	spi_max(MAX_DIGIT3, 0);
	spi_max(MAX_DIGIT4, 0);
	spi_max(MAX_DIGIT5, 0);
	spi_max(MAX_DIGIT6, 0);
	spi_max(MAX_DIGIT7, 0);
	spi_max(MAX_SHUTDOWN, 1); 	// Wake oscillators/display up
        
        
      while(1)
      {
        sicaklik_temp=sicaklik;
        
        d4=sicaklik_temp%10;
        sicaklik_temp=sicaklik_temp/10;
         
        d3=sicaklik_temp%10;
        sicaklik_temp=sicaklik_temp/10;
         
        d2=sicaklik_temp%10;
        sicaklik_temp=sicaklik_temp/10;
         
        d1=sicaklik_temp%10;
        
        keypad();
        keycontrol();
        
        sicaklik_goster();
        saatgoster();
      }

}

// Enable harware SPI
void spi_init()
{
	UCA0CTL1 |= UCSWRST; 		// USCI in Reset State (for config)
	// Leading edge + MSB first + Master + Sync mode (spi)
	UCA0CTL0 = UCCKPH + UCMSB + UCMST + UCSYNC;
	UCA0CTL1 |= UCSSEL_2; 		// SMCLK as clock source
	UCA0BR0 |= 0x01; 		// SPI speed (same as SMCLK)
	UCA0BR1 = 0;
	P1SEL |= SPI_SIMO + SPI_CLK;	// Set port pins for USCI
	P1SEL2 |= SPI_SIMO + SPI_CLK;
	UCA0CTL1 &= ~UCSWRST; 		// Clear USCI Reset State
}

void sicaklik_init() //ilklendirme
{
ADC10CTL0&=~ENC; // ilgili biti 0la conv yapma
ADC10CTL0=SREF_1 + REFON + ADC10ON + ADC10SHT_3 ; //1.5V ref,Ref on,64 clocks for sample
ADC10CTL1=INCH_10+ ADC10DIV_3; // temp sensor is at 10 and clock/4
__delay_cycles(256);

}

void spi_max(uint8_t address, uint8_t data)
{

        UCA0TXBUF = address ;	
	while (UCA0STAT & UCBUSY);		// Wait until done
	UCA0TXBUF = data;			// Send byte of data
	while (UCA0STAT & UCBUSY);
	P1OUT |= SPI_CS;			// /CS inactive or Load high
#ifndef USE_MAX7219
	P1OUT &= ~(SPI_CS);			// MAX7219 pulses Load high
#endif
}



void sicaklik_oku()
{
  if(count%2==0){
    float yeni=0;
    ADC10CTL0 |= ENC + ADC10SC;      //enable conversion and start conversion
    while(ADC10CTL1 & BUSY);         //wait..i am converting..pum..pum..
    sicaklik_ham = ADC10MEM;
    ADC10CTL0&=~ENC;                    //disable adc conv

    yeni = (((sicaklik_ham-673)*423)/1024.0f)*100;
    sicaklik1=(unsigned int)yeni;
  }
    if(count%2==1){
    float yeni=0;
    ADC10CTL0 |= ENC + ADC10SC;      //enable conversion and start conversion
    while(ADC10CTL1 & BUSY);         //wait..i am converting..pum..pum..
    sicaklik_ham = ADC10MEM;
    ADC10CTL0&=~ENC;                    //disable adc conv

    yeni = (((sicaklik_ham-673)*423)/1024.0f)*100;
    sicaklik2=(unsigned int)yeni;
  }
    sicaklik=(sicaklik1+sicaklik2)/2;
    count++;
}



void sicaklik_goster(void)
{
 
 
 spi_max(MAX_DIGIT0, number[d1]);
__delay_cycles(700);    

 spi_max(MAX_DIGIT1, number[d2]+0x80);
__delay_cycles(700);     

 spi_max(MAX_DIGIT2, number[d3]);
__delay_cycles(700);      

  spi_max(MAX_DIGIT3, number[d4]);
__delay_cycles(700);      

}




 // Timer A0 Kesme Vektörü
 #pragma vector=TIMER0_A0_VECTOR
 __interrupt void TA0_ISR (void)
 {
    i++;
  
    if (i>=3)
    { 
        saniyeartir();   
        i = 0;
    }
    
 }



void saniyeartir(void) //saat fonksiyonu
 
 {
 
      saniye++;
      sicaklik_oku();
  
  if(saniye==60)
      {
        saniye=0;
        dakika++;
 
            if(dakika==60)
                {
                  dakika=0;
                  saat++;
                       
                          if(saat==24)
                            saat=0;
                 }
      }
  }

void saatgoster(void) 
 
{
      
   if(saat==24)
   {
     saat=0;
   }
   
   
   if(dakika==60)
   {
     dakika=0;
     saat++;
   }
      
     digit[0]=saat/10;
     digit[1]=saat%10;
     digit[2]=dakika/10;
     digit[3]=dakika%10;

    spi_max(MAX_DIGIT7,number[digit[3]]);
    __delay_cycles(500);
 
    spi_max(MAX_DIGIT6,number[digit[2]]);
     __delay_cycles(500);
 
    spi_max(MAX_DIGIT5,number[digit[1]]+0x80); // 0x80 for dot.
     __delay_cycles(500);
 
    spi_max(MAX_DIGIT4,number[digit[0]]);
     __delay_cycles(500);
 
    }


void keycontrol(void)
{
    if(tus=='1')
      {
       saat++;
          if(saat==24){saat=0;}
              tus='0';

          __delay_cycles(99999);
      }
  
    
    if(tus=='3')
       { 
        dakika++;
           if(dakika==60){dakika=0;}
               tus='0';
            
           __delay_cycles(99999);
       }

     
   
    if(tus=='A')
       {
          saat=0;
          dakika=0;
          tus='0';
         
          __delay_cycles(99999);
       }

    if(tus=='2')
       {
         saat=2;
         tus='0';
        __delay_cycles(99999);
       }

           if(tus=='4')
      {
       saat++;
          if(saat==24){saat=0;}
              tus='0';

          __delay_cycles(99999);
      }
  
    
    if(tus=='5')
       { 
        dakika++;
           if(dakika==60){dakika=0;}
               tus='0';
            
           __delay_cycles(99999);
       }

     
   
    if(tus=='6')
       {
          saat=0;
          dakika=0;
          tus='0';
         
          __delay_cycles(99999);
       }

    if(tus=='B')
       {
         saat=2;
         tus='0';
        __delay_cycles(99999);
       }
    
    
    if(tus=='7')
      {
       saat++;
          if(saat==24){saat=0;}
              tus='0';

          __delay_cycles(99999);
      }
  
    
    if(tus=='8')
       { 
        dakika++;
           if(dakika==60){dakika=0;}
               tus='0';
            
           __delay_cycles(99999);
       }

     
   
    if(tus=='9')
       {
          saat=0;
          dakika=0;
          tus='0';
         
          __delay_cycles(99999);
       }

    if(tus=='C')
       {
         saat=2;
         tus='0';
        __delay_cycles(99999);
       }


    if(tus=='*')
      {
       saat++;
          if(saat==24){saat=0;}
              tus='0';

          __delay_cycles(99999);
      }
  
    


     
   
    if(tus=='#')
       {
          saat=0;
          dakika=0;
          tus='0';
         
          __delay_cycles(99999);
       }

    if(tus=='D')
       {
         saat=2;
         tus='0';
        __delay_cycles(99999);
       }
}




void keypad (void)
{

if((P1IN&0x01)==0 )
{       satir=0;
        P2OUT |= 0x02;
          if ((P1IN & 0x01)==0x01){
                sutun=0;
                tus=matrix[satir][sutun];
                __delay_cycles(9999);
                }
        P2OUT &=~0x02;
        P2OUT |= 0x20;
           if ((P1IN & 0x01)==0x01){
                sutun=1;
                tus=matrix[satir][sutun];
                __delay_cycles(9999);
                 }
        
        P2OUT &=~0x20;
        P1OUT |= 0x40;
            if ((P1IN & 0x01)==0x01){
                sutun=2;
                tus=matrix[satir][sutun];
                __delay_cycles(9999);
                 }
       
      P1OUT &=~0x40;
        P1OUT |= 0x80;
            if ((P1IN & 0x01)==0x01){
                sutun=3;
                tus=matrix[satir][sutun];
                __delay_cycles(9999); 
            }
            P1OUT &=~0x80;
}



if( (P1IN&0x02)==0)
{       satir=1;
        P2OUT |= 0x02;
          if ((P1IN&0x02)==0x02){
                sutun=0;
                tus=matrix[satir][sutun];
                __delay_cycles(9999);
                 }
        P2OUT &=~0x02;
        P2OUT |= 0x20;
           if ((P1IN&0x02)==0x02){
                sutun=1;
                tus=matrix[satir][sutun];
                __delay_cycles(9999);
                 }
        
        P2OUT &=~0x20;
        P1OUT |= 0x40;
            if ((P1IN&0x02)==0x02){
                sutun=2;
                tus=matrix[satir][sutun];
                __delay_cycles(9999);
                }
       
        P1OUT &=~0x40;
        P1OUT |= 0x80;
            if ((P1IN&0x02)==0x02){
                sutun=3;
               tus=matrix[satir][sutun];
               __delay_cycles(9999);
                 }
            P1OUT &=~0x80;
}




if((P2IN&0x04)==0)
{       satir=2;
        P2OUT |= 0x02;
          if ((P2IN & 0x04)==0x04){
                sutun=0;
                tus=matrix[satir][sutun];
                __delay_cycles(9999);
                 }
        P2OUT &=~0x02;
        P2OUT |= 0x20;
           if ((P2IN & 0x04)==0x04){
                sutun=1;
                tus=matrix[satir][sutun];
                __delay_cycles(9999);
                }
        
        P2OUT &=~0x20;
        P1OUT |= 0x40;
            if ((P2IN & 0x04)==0x04){
                sutun=2;
                tus=matrix[satir][sutun];
                __delay_cycles(9999);
                 }
       
        P1OUT &=~0x40;
        P1OUT |= 0x80;
            if ((P2IN & 0x04)==0x04){
                sutun=3;
                tus=matrix[satir][sutun];
                __delay_cycles(9999);
                 }
            P1OUT &=~0x80;
}




if( (P2IN&0x08)==0)
{       satir=3;
        P2OUT |= 0x02;
          if ((P2IN & 0x08)==0x08){
                sutun=0;
                tus=matrix[satir][sutun];
                __delay_cycles(9999);
                 }
        P2OUT &=~0x02;
        P2OUT |= 0x20;
           if ((P2IN & 0x08)==0x08){
                sutun=1;
                tus=matrix[satir][sutun];
                __delay_cycles(9999);
                 }
        
        P2OUT &=~0x20;
        P1OUT |= 0x40;
            if ((P2IN & 0x08)==0x08){
                sutun=2;
                tus=matrix[satir][sutun];
                __delay_cycles(9999);
                 }
       
        P1OUT &=~0x40;
        P1OUT |= 0x80;
            if ((P2IN & 0x08)==0x08){
                sutun=3;
            tus=matrix[satir][sutun];
            __delay_cycles(9999);
            }
            P1OUT &=~0x80;
}

}


