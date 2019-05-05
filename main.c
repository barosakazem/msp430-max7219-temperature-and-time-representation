
#include "io430.h"
#include "stdint.h"


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
int digit[4];

// Function prototypes
void spi_init();
void spi_max(unsigned char address, unsigned char data);
void sicaklik_init(void);
void sicaklik_oku(void);
void sicaklik_goster();
void saatayarla(int);
int count=0;
int count2=0;
const uint8_t number[] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90
	
};
const uint8_t disp[] = {MAX_DIGIT0, MAX_DIGIT1, MAX_DIGIT2, MAX_DIGIT3, MAX_DIGIT4, MAX_DIGIT5, MAX_DIGIT6, MAX_DIGIT7
};
 unsigned int i=0,saniye=0,dakika=0,saat=0,mod=0;
void saatgoster();
void saniyeartir(void);
unsigned int d1,d2,d3,d4,sicaklik_temp,sicaklik_ham;
unsigned int sayi=0;
unsigned int sicaklik=0;
volatile char tick;
// Program start
int main(void)
{
  P2DIR=0x00;
  P2REN=0x03;
  P2OUT=0x03;
  P2IE=0x03;
  P2IES=0x03;
  
  
	WDTCTL = WDTPW + WDTHOLD; 	// Disable WDT
	DCOCTL = CALDCO_1MHZ; 		// 1 Mhz DCO
	
       BCSCTL1 = CALBC1_1MHZ;

TACCR0 = 41000;
  TACCTL0 = CCIE;
  TACTL = MC_1 + ID_3 + TASSEL_2 + TACLR; 
   
 sicaklik_init();
	// Setup Port1 pins
	P1DIR |= SPI_SIMO + SPI_CLK + SPI_CS;
	#ifdef USE_MAX7219
	P1OUT |= SPI_CS;		// MAX7219 Chip Select is inactive high
	#endif

	spi_init();			// Init USCI in SPI mode
__bis_SR_register(GIE);
	// Initialise MAX7219 with 8x8 led matrix
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
        

	// Ready to start displaying something!

ADC10CTL0 |= ENC + ADC10SC;
	// Loop forever
	while (1) 
	{ 
           
      
       
         
	 sicaklik_temp=sicaklik;
        
        d4=sicaklik_temp%10;
        sicaklik_temp=sicaklik_temp/10;
         
        d3=sicaklik_temp%10;
        sicaklik_temp=sicaklik_temp/10;
         
        d2=sicaklik_temp%10;
        sicaklik_temp=sicaklik_temp/10;
         
        d1=sicaklik_temp%10;
        
        if((count%2)==0){sicaklik_goster();}
       
       if((count%2)!=0){saatgoster();}
       
	}

}

 // Timer A0 Kesme Vektörü
 #pragma vector=TIMER0_A0_VECTOR
 __interrupt void TA0_ISR (void)
 {
 i++;
  
  if (i>=3)
  {  saniyeartir();   
    i = 0;
                           
 
  }
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
ADC10CTL0= SREF_1 + ADC10SHT_3 + REFON + ADC10ON + MSC+ ADC10IE;
ADC10CTL1= INCH_10 + ADC10SSEL_0 + ADC10DIV_0 + CONSEQ_2 ;
ADC10DTC1=32; //ardisik 32 veri oku ort al noise elimine 
__delay_cycles(256);

}

void sicaklik_oku()
{

float yeni=0;

sicaklik_ham = ADC10MEM;
yeni = (((sicaklik_ham-673)*423)/1024.0f)*100;
sicaklik=(unsigned int)yeni;

}
void sicaklik_goster(void)
{
 
 
 spi_max(disp[0], number[d1]);
__delay_cycles(700);    
 spi_max(disp[1], number[d2]+0x80);
__delay_cycles(700);       
 spi_max(disp[2], number[d3]);
__delay_cycles(700);         
  spi_max(disp[3], number[d4]);
__delay_cycles(700);      

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

void saatgoster(void) //tarama fonksiyonu
 
    {
     digit[0]=saat/10;
     digit[1]=saat%10;
     digit[2]=dakika/10;
     digit[3]=dakika%10;

    spi_max(MAX_DIGIT7,number[digit[3]]);
    __delay_cycles(500);
 
    spi_max(MAX_DIGIT6,number[digit[2]]);
     __delay_cycles(500);
 
    spi_max(MAX_DIGIT5,number[digit[1]]+0x80);
     __delay_cycles(500);
 
    spi_max(MAX_DIGIT4,number[digit[0]]);
     __delay_cycles(500);
 
    }

void saatayarla(int ccount2)
{
  if(ccount2==1)
  {
  saat=saat+10;
  }

  if(ccount2==2)
  {
  saat++;
  }
  if(ccount2==3)
  {
  dakika=dakika+10;
  }
  if(ccount2==4)
  {
  dakika++;
  }
  
}

#pragma vector=PORT2_VECTOR
__interrupt void P2_ISR(void)
{
__delay_cycles(90);
if((P2IFG & 0x01) && (P2IN &0x01)==0)
{count++;


}

if(P2IFG & 0x02 && (P2IN &0x02)==0)
{count2++;
if(count2==5){count2=0;}
saatayarla(count2);
}
P2IFG=0x00;
}

