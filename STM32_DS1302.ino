//Follows the datasheet for absolute minimum of timer functions, easy to integrate in any code.

//This code uses the PA13 PA14 ports freeing up PA13 (SWDIO) and PA14 (SWCLK) duties disableDebugPorts();
//To change ports, search Github "STM32 GPIO registers cheatsheet"
//Uses direct port access, probably just for fun, because the DS1302 needs 1 usec delays anyway.
//Tried to avoid digitalWrite .. digitalRead 


#define PA14   PA14   // Serial Clock 
#define PA13   PA13   // Data I/O
#define PB1    PB1    // Chip Enable

#define bcd2bin(h,l)    (((h)*10) + (l))
#define bin2bcd_h(x)   ((x)/10)
#define bin2bcd_l(x)    ((x)%10)


/* Register names.
// Since the highest bit is always '1', 
// the registers start at 0x80
// If the register is read, the lowest bit should be '1'.
#define DS1302_SECONDS           0x80
#define DS1302_MINUTES           0x82
#define DS1302_HOURS             0x84
#define DS1302_DATE              0x86
#define DS1302_MONTH             0x88
#define DS1302_DAY               0x8A
#define DS1302_YEAR              0x8C
#define DS1302_ENABLE            0x8E
#define DS1302_TRICKLE           0x90
#define DS1302_CLOCK_BURST       0xBE
#define DS1302_CLOCK_BURST_WRITE 0xBE
#define DS1302_CLOCK_BURST_READ  0xBF
#define DS1302_RAMSTART          0xC0
#define DS1302_RAMEND            0xFC
#define DS1302_RAM_BURST         0xFE
#define DS1302_RAM_BURST_WRITE   0xFE
#define DS1302_RAM_BURST_READ    0xFF

*/


byte rtc[8];

void setup()
{ 
  disableDebugPorts();
  Serial.begin(9600);for (int i=0; i<999; i+=2) Serial.println(i);  //serial print warmup, while(!Serial) does not work
  rtcWr (0x8e, 0x00); rtcWr (0x90, 0x00);//  Enable clock /  Disable Trickle Charger.
  rtc[0]=0x32;  rtc[1]=0x00; rtc[2]=0x13; // seconds,min, hour
  rtc[3]=0x23; rtc[4]=0x01;  rtc[5]=0x05; rtc[6]=0x21; // date, month, day, year
  for (byte i=0; i<7; i++) { rtcWr(0x80 | i*2,rtc[i]);  //array initial write
                            Serial.print(rtc[i],HEX); Serial.print("=");Serial.print(0x80|i*2,HEX); Serial.print(" "); 
                           }
}


void loop()
{
for (byte i=0; i<7; i++) rtc[i]=rtcRd(i*2+0x80);
for (byte i=0; i<7; i++) { Serial.print(rtc[6-i],HEX); Serial.print(" "); }
delay( 1000); Serial.println();
}

byte rtcRd(int address)
{
  byte data; 
  busStart();
  twiWr( address | 0x01, 1);  //read bit set
  data =twiRd();
  busStop();
  return (data);
}

void rtcWr( int address, byte data)
{
 address &= 0xfe;  // clear low bit in address
 busStart();       
 twiWr( address, 0); // keep I/O-line
 twiWr( data, 0); 
 busStop();  
}

void busStart( void)
{ GPIOB->regs->BRR =0x0002 ; //digitalWrite( PB1, LOW); // default, not enabled
  GPIOB->regs->CRL= (GPIOB->regs->CRL & 0xffffff0f) | 0x00000030; // pinMode( PB1, OUTPUT);
  GPIOA->regs->CRH= (GPIOA->regs->CRH & 0xf00fffff) | 0x03300000;  // pinMode( PA13, OUTPUT); //  pinMode( PA14, OUTPUT);
  GPIOA->regs->BRR =0x4000 ; //digitalWrite( PA14, LOW); // default, clock low
  GPIOB->regs->BSRR =0x0002 ; //digitalWrite( PB1, HIGH); // start the session
  delayMicroseconds(4);       
}

void busStop(void)
{ GPIOB->regs->BRR =0x0002 ; //digitalWrite( PB1, LOW);   // CE
  delayMicroseconds(4);   
}

byte twiRd( void)
{
  byte i, rd = 0;
  for( i = 0; i <= 7; i++)
  { GPIOA->regs->BSRR =0x4000 ; delayMicroseconds(1);//14th bit //digitalWrite( PA14, HIGH);
    GPIOA->regs->BRR  =0x4000 ; delayMicroseconds(1);//14th bit//digitalWrite( PA14, LOW);     
    rd |= ((GPIOA->regs->IDR & 0x2000) > 0)<<i; //data |= digitalRead(PA13)<<i; //bitWrite( data, i, digitalRead( PA13));
  }
  return( rd);
}

void twiWr( byte wr, byte r)
{
   for (byte i = 0; i <= 7; i++)
  {  if (wr & (1<<i)) GPIOA->regs->BSRR =0x2000 ; else GPIOA->regs->BRR =0x2000 ;
    //digitalWrite(PA13, wr & (1<<i));   //digitalWrite(PA13, bitRead(data, i));  
    delayMicroseconds( 1);     //digitalWrite( PA14, HIGH);     
    GPIOA->regs->BSRR =0x4000 ; //14th bit set
    delayMicroseconds(1);   
    //if( r && i==7) pinMode( PA13, INPUT);
    if( r && i==7) GPIOA->regs->CRH= (GPIOA->regs->CRH & 0xff0fffff) | 0x00800000; // set A13 input
    else { GPIOA->regs->BRR =0x4000 ; delayMicroseconds(1); }    
  }
}
