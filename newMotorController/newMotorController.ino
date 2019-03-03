//#include <SPI.h>

//LEO PIN 8 should be SS for the Cat5 cable
//Motor is lacking a clock currently(which clock is this referring to?)

void setup() {
  // put your setup code here, to run once:
  
  Serial.begin(9600);   // debugging
  
  // have to send on master in, *slave out*

  //pinMode(MOSI, INPUT);
  
  // turn on SPI in slave mode
  spi_enable();

  // turn on interrupts
  SPCR |= bit(SPIE);

  //TODO: setup correct SPI phase and polarity to match DACs
  
  //See also:
  //https://gist.github.com/argon/4578068
  
  sck_detector_init();
}


// ------------------------------ Receive Timer ------------------------------
/* Using Timer/Counter0 for SPI timeout(8-bit timer)
 *  It takes about 32 us to receive a byte over SPI.
 *  This timer configured for clk/8 yields 128 us for a timer overflow(counting from 0 to 255)
 *  at 16 MHz system clock. Even clk/64 yielding 1.024 ms may be fine given the low update rate.
 */

// has no effect if already started(continues running toward timeout)
static void rxtimer_start(void) {
  TCCR0B = 2;               // counter running at clk/8
}

static void rxtimer_stop(void) {
  TCCR0B = 0;               // counter not running
  TCNT0 = 0;                // reset counter
  TIFR0 = bit(TOV0);        // reset overflow flag
}

static bool rxtimer_timeout(void) {
  return TIFR0 & bit(TOV0);
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
  SPCR |= bit(SPE);
}

static void spi_disable(void) {
  SPCR &= ~bit(SPE);
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

void loop(void) {
  
  //Serial.println(SPDR);

  if(rx_byte_count == 2) {
    rx_reset();

    // TODO: do something useful with rx_word
    //buf[pos] = 0;  
    Serial.println(rx_word);
    //pos = 0;

  } else if(rxtimer_timeout || rx_byte_count > 2) {
    rx_reset();
  } else if(sck_detected) {
    rxtimer_start();
    //sck_detector_reset();     // TODO: may be unnecessary
  }

}  // end of loop
