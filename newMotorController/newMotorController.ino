#include <SPI.h>
//BEGIN
//https://gist.github.com/argon/4578068

//LEO PIN 8 should be SS for the Ethernet cable
//Motor is lacking a clock currently


char buf [100];
volatile byte pos;
volatile boolean process_it;

void setup() {
  // put your setup code here, to run once:
  SetupForServo();
}

//SPCR 0x002c 
//SPSR 0x002d
//SPDR 0x002e

void SetupForServo(){



  SPCR << 0xC0; 

 

  // bit 7 SPI Interrupt Enable = 1

  // bit 6 SPI Enable =1

  // bit 5 DORD: Data Order MSB First

  // bit 4 MSTR: Master Slave select configured as slave

  // bit 3 CPOL Clock polarity transfer on rising 

  // bit 2 Clock Phase Sample on Leading

  // bit 1&0 clock for master don't care



  //TODO:

  //Place ISR Start Address at $0030 

  // 0x0030; assem ORG origin

  

   __asm__                                 \

    (                                      \

        "SEI"   "\n\t"                     \

     );           



}

void SetupForServo1(){
  

  Serial.begin (9600);   // debugging
  // have to send on master in, *slave out*

  pinMode(MOSI, INPUT);
//Turn on SPI in slave mode
SPCR |= _BV(SPE);

// get ready for an interrupt 
pos = 0;   // buffer empty
process_it = false;


//See also:
//https://gist.github.com/argon/4578068

//  attachInterrupt(digitalPinToInterrupt(pin), ISR, mode);
SPI.attachInterrupt();


  SPCR << 0xC0 & 0x7F; 
 
  // bit 7 SPI Interrupt Enable = 1
  // bit 6 SPI Enable =1
  // bit 5 DORD: Data Order MSB First
  // bit 4 MSTR: Master Slave select configured as slave
  // bit 3 CPOL Clock polarity transfer on rising 
  // bit 2 Clock Phase Sample on Leading
  // bit 1&0 clock for master don't care

  //TODO:
  //Place ISR Start Address at $0030 
  // 0x0030; assem ORG origin
  
  //Allow the slave to generate an interrupt upon SPI data register full
   __asm__                                 \
    (                                      \
        "SEI"   "\n\t"                     \
     );           

}

// SPI interrupt routine
// This is automatically called by the Device Serial Peri Register is full
ISR (SPI_STC_vect)

{

byte c = SPDR;  // grab byte from SPI Data Register

  

  // add to buffer if room

  if (pos < sizeof buf)

    {

    buf [pos++] = c;

    

    // example: newline means time to process buffer

    if (c == '\n')

      process_it = true;

      

    }  // end of room available

}  // end of interrupt routine SPI_STC_vect



//END https://gist.github.com/argon/4578068
void loop (void)

{
    Serial.println(SPDR);

  if (process_it)

    {

    buf [pos] = 0;  

    Serial.println (buf);

    pos = 0;

    process_it = false;

    }  // end of flag set

    

}  // end of loop
