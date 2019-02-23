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
  SetupForServo1();
}

//SPCR 0x002c 
//SPSR 0x002d
//SPDR 0x002e

/*void SetupForServo(){



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



}*/


void SetupForServo1(){
  
  /* Using Timer/Counter0 for SPI timeout (8-bit timer)
   *  It takes about 32 us to receive a byte over SPI.
   *  This timer configured for clk/8 yields 128 us for a timer overflow (counting from 0 to 255)
   *  at 16 MHz system clock. Even clk/64 yielding 1.024 ms may be fine given the low update rate.
   */
  TCNT0 = 255;  // force overflow initially, to avoid false detection of second byte by SPI ISR
  TCCR0B = 2;   // clk/8
  

  Serial.begin (9600);   // debugging
  
  // have to send on master in, *slave out*

  //pinMode(MOSI, INPUT);
  
  // turn on SPI in slave mode
  SPCR |= _BV(SPE);

  // turn on interrupts
  SPCR |= _BV(SPIE);
  
  // get ready for an interrupt 
  //pos = 0;   // buffer empty
  process_it = false;
  
  
  //See also:
  //https://gist.github.com/argon/4578068
  
  //  attachInterrupt(digitalPinToInterrupt(pin), ISR, mode);
  SPI.attachInterrupt();


  //SPCR << 0xC0 & 0x7F; 
 
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
 /*  __asm__                                 \
    (                                      \
        "SEI"   "\n\t"                     \
     );           */

  
}

uint16_t rx_word = 0;

// SPI interrupt routine
// This is automatically called by the Device Serial Peri Register is full

ISR (SPI_STC_vect)
{
  rx_word = (rx_word << 8) | SPDR;
  
  if(TIFR0 & _BV(TOV0)) {   // overflow? this must be the first byte of a word, so set up to receive the second one   
    TCNT0 = 0;                // reset counter
    TIFR0 = _BV(TOV0);        // reset overflow flag
  } else {                  // no overflow? then this is the second byte
    TCNT0 = 255;              // force an overflow to guarantee the next byte is interpreted as the first byte of a word
    process_it = true;        // use the received word
  }
 
  /*byte c = SPDR;

  // add to buffer if room
  if (pos < sizeof buf) {
    buf [pos++] = c;
   
    // example: newline means time to process buffer
    if (c == '\n')
      process_it = true;
     
  }  // end of room available
  */
}


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
