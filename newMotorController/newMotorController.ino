//#include <SPI.h>

//LEO PIN 8 should be SS for the Cat5 cable
//Motor is lacking a clock currently(which clock is this referring to?)



void setup() {
  // put your setup code here, to run once:
  
  Serial.begin(9600);   // debugging
  
  while(!Serial){
      ; // waiting for connection
  }

  Serial.println("Up and running 1");

  rx_init();
}

static void rx_init(void) {
  // have to send on master in, *slave out*

  //pinMode(MOSI, INPUT);

  //delay(1000);
  
  // turn on SPI in slave mode
  spi_enable();

  // turn on interrupts
  //SPCR |= bit(SPIE);

  //TODO: setup correct SPI phase and polarity to match DACs
  
  //See also:
  //https://gist.github.com/argon/4578068
  
  sck_detector_init();

  // Arduino apparently likes to mess with stuff behind your back.
  // TCCR1A and TCCR1C are 0 on reset according to the Atmel datasheet,
  // but apparently must be manually initialized when dealing with Arduino.
  TCCR1A = 0;
  TCCR1C = 0;
  
  rxtimer_stop();       // initialize timer  
}


// ------------------------------ Receive Timer ------------------------------
/* Using Timer/Counter1 for SPI timeout(8-bit timer)
 *  It takes about 32 us to receive a byte over SPI.
 *  This timer configured for clk/8 yields 128 us for a timer overflow(counting from -256 to 0)
 *  at 16 MHz system clock.
 */

// has no effect if already started(continues running toward timeout)
static void rxtimer_start(void) {
  TCCR1B = bit(CS11);       // counter running at clk/8 (2 MHz)       FIXME
}

static void rxtimer_stop(void) {
  TCCR1B = 0;               // counter not running
  TCNT1H = 0xFF;            // reset counter to -256
  TCNT1L = 0x00;
  TIFR1 = bit(TOV1);        // reset overflow flag
}

static bool rxtimer_timeout(void) {
  return (TIFR1 & bit(TOV1)) == bit(TOV1);
}


// ------------------------------ SPI SCK Detector ------------------------------
// NOTE: Make sure no other PCINTs are used.
static void sck_detector_init(void) {
  PCMSK0 = bit(PCINT1);
}

static void sck_detector_reset(void) {
  PCIFR = bit(PCIF0);
}

// Indicates whether any activity occurred on the SPI SCK line since the last call to sck_detector_reset
// NOTE: PCINT1 is the only one of PCINT7..0 to be enabled; otherwise this test would be ambiguous
static bool sck_detected(void) {
  return(PCIFR & bit(PCIF0)) == bit(PCIF0);
}


// ------------------------------ SPI ------------------------------
volatile uint16_t rx_word = 0;
volatile int rx_byte_count = 0;

static void spi_enable(void) {
  SPCR = bit(SPE) | bit(SPIE);
}

static void spi_disable(void) {
  SPCR = 0; //&= ~(bit(SPE) | bit(SPIE));
}

// SPI interrupt routine
// This is automatically called by the Device Serial Peri Register is full

ISR(SPI_STC_vect) {
  rx_word =(rx_word << 8) | SPDR;
  rx_byte_count++;
}


// ------------------------------ Main ------------------------------

static void rx_reset(void) {
  spi_disable();
  rxtimer_stop();
  rx_byte_count = 0;
  sck_detector_reset();
  spi_enable();
}

typedef union {
  struct {
    uint16_t position :13;
    uint16_t sync     : 2;
    uint16_t strobe   : 1;
  };
  uint16_t word;
} Packet;

Packet packet;

void loop(void) {
  
  //Serial.println(SPDR);

  if(rx_byte_count == 2) {
    // TODO: do something useful with rx_word
    //buf[pos] = 0;  
    //delay(100);
    packet.word = rx_word;
    Serial.print(packet.strobe);
    Serial.print(" ");
    Serial.print(packet.sync);
    Serial.print(" ");
    Serial.println(packet.position);
    rx_reset();


  } else if(rxtimer_timeout() || rx_byte_count > 2) {
    if(rxtimer_timeout()) {
      Serial.println("timeout");
    }
    rx_reset();
  } else if(sck_detected()) {
    rxtimer_start();
    //sck_detector_reset();     // TODO: may be unnecessary
  }

}  // end of loop
