//#include <SPI.h>

//LEO PIN 8 should be SS for the Cat5 cable
//Motor is lacking a clock currently(which clock is this referring to?)

/**************************************
// PORTB
#define PB_ENCODER_SDI 7
#define PB_ENCODER_SDO 6    // input
#define PB_MOTOR_CS    5
#define PB_MOTOR_CLK   4
                            // PB0-2 used by SPI

// PORTC
#define PC_ENCODER_CS  7

// PORTD
#define PD_ENCODER_CLK 6
#define PD_STROBE_LED  0

// PORTE
#define PE_MOTOR_SDI   6
***************************************/

//#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(x)  Serial.println (x)
#define DEBUG_DELAY(x) delay(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_DELAY(x)
#endif

/* COUNTER AND MOTOR */
#define CS0_PIN 13      /* CS0 is encoder */
#define SCK0_PIN  12
#define MISO0_PIN  11
#define MOSI0_PIN  10 /* input */
#define CS1_PIN  9      /* CS1 is motor */
#define SCK1_PIN  8
#define MISO1_PIN  7
#define MOTOR_SATURATION 1024
#define STROBE_PIN 3
/* END COUNTER AND MOTOR */

#define ENCODER_COUNTS 0x1756

void writeMotor(int raw_value) {

  int value = raw_value;

  /* LOW BYTE HIGH BYTE*/
  if (value > MOTOR_SATURATION) {
    value = MOTOR_SATURATION - 1;
  }
  if (value < -MOTOR_SATURATION) {
    value = -MOTOR_SATURATION + 1;
  }
  DEBUG_PRINT("(PWM) Motor Drive:");
  DEBUG_PRINT(value);

  digitalWrite(CS1_PIN, LOW);
  shiftOut(MISO1_PIN/*dataPin*/, SCK1_PIN/*clockPin*/, MSBFIRST, highByte(value));
  shiftOut(MISO1_PIN/*dataPin*/, SCK1_PIN/*clockPin*/, MSBFIRST, lowByte(value));
  digitalWrite(CS1_PIN, HIGH);
};


int drive_value_multiplied = 0;

void goToPosition(float x) {
  DEBUG_PRINT("(Command) Desired Position:");
  DEBUG_PRINT(x);
  float position = read_counter();
  DEBUG_PRINT("(Actual) Position Value:");
  DEBUG_PRINT(position);
  float error_value = x - position;
  DEBUG_PRINT("(Error) Error In Degrees:");
  DEBUG_PRINT(error_value);
  const float gain = 2;
  float calculated_error_value = calculated_error(error_value);
  DEBUG_PRINT("(Error After Correction) Calculated Error Value In Degrees:");
  DEBUG_PRINT(error_value);
  drive_value_multiplied = calculated_error_value * gain;
  writeMotor(drive_value_multiplied);
}

float calculated_error(float error_value) {

  float result = error_value;
  if (error_value <= -ENCODER_COUNTS/2) {
    result += ENCODER_COUNTS ;
  }
  if (error_value >= ENCODER_COUNTS/2) {
    result -= ENCODER_COUNTS;
  }
  return result;
}


void init_counter_clear() {
  /* Setup for write to MDR */
  const int WRITE_MDR_0  = 0x88;
  const int MDR_0_SETUP_4_X_2X_CLOCK = 0xE3;
  const int WRITE_MDR_1 = 0x90;
  //  const int MDR_0_DATA_LATCH = 0xF3;
  digitalWrite(CS0_PIN, LOW);

  shiftOut(MISO0_PIN, SCK0_PIN, MSBFIRST, WRITE_MDR_0);
  shiftOut(MISO0_PIN, SCK0_PIN, MSBFIRST, MDR_0_SETUP_4_X_2X_CLOCK);

  digitalWrite(CS0_PIN, HIGH);
  digitalWrite(CS0_PIN, LOW);

  shiftOut(MISO0_PIN, SCK0_PIN, MSBFIRST, WRITE_MDR_1);
  shiftOut(MISO0_PIN, SCK0_PIN, MSBFIRST, 0x02/*Storing a value in the register*/);

  digitalWrite(CS0_PIN, HIGH);
}

byte read_str() {
  digitalWrite(CS0_PIN, LOW);
  shiftOut(MISO0_PIN, SCK0_PIN, MSBFIRST, 0x70);
  byte result =  shiftIn(MOSI0_PIN, SCK0_PIN, MSBFIRST);
  digitalWrite(CS0_PIN, HIGH);
  return result;
}


int16_t read_counter() {

  digitalWrite(CS0_PIN, LOW);
  shiftOut(MISO0_PIN/*dataPin*/, SCK0_PIN/*clockPin*/, MSBFIRST, 0x60);
  uint8_t result1 =  shiftIn(MOSI0_PIN, SCK0_PIN, MSBFIRST);
  uint8_t result2 =  shiftIn(MOSI0_PIN, SCK0_PIN, MSBFIRST);
  digitalWrite(CS0_PIN, HIGH);

  int16_t s = (((uint16_t)result1 << 8) | result2);
  if (s < 0) {
    s += ENCODER_COUNTS;
  }
  return s;
}

void wait_for_index(){
  byte x;
  do {
    // optional delay
    x = read_str();
  } while((x & 0x10) == 0x00);
  /*byte x = read_str();
  byte mask = 0x10;
  while(!(x & mask)){
    //DEBUG_PRINT(read_counter());
    //delay(50);
    delay(0);
    x = read_str();
  }*/
 }

void clear_status_register(){
  const int WRITE_MDR0_CLEAR_REGISTER = 0x30;
  digitalWrite(CS0_PIN, LOW);
  shiftOut(MISO0_PIN, SCK0_PIN, MSBFIRST, WRITE_MDR0_CLEAR_REGISTER);
  digitalWrite(CS0_PIN, HIGH);   
}

void home_motor(){
    init_counter_clear();
    clear_status_register();
    writeMotor(200);
    wait_for_index();
    writeMotor(0);
    DEBUG_PRINT("MOTOR HOMED");
}

void initializeClock() {
  digitalWrite(SCK0_PIN, LOW);
  digitalWrite(SCK1_PIN, LOW);
}

void initializeChipSelect() {
  digitalWrite(CS0_PIN, HIGH);
  digitalWrite(CS1_PIN, HIGH);
}

void initialize_pins() {
  pinMode(STROBE_PIN, OUTPUT);
  digitalWrite(STROBE_PIN, LOW);
  pinMode(CS0_PIN, OUTPUT);
  pinMode(SCK0_PIN, OUTPUT);
  pinMode(MISO0_PIN, OUTPUT);
  pinMode(MOSI0_PIN, INPUT);
  pinMode(CS1_PIN, OUTPUT);
  pinMode(SCK1_PIN, OUTPUT);
  pinMode(MISO1_PIN, OUTPUT);
}


void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);   // debugging
  
  /*while(!Serial){
      ; // waiting for connection
  }*/

  Serial.println("Up and running 4");

  initialize_pins();
  initializeChipSelect();
  initializeClock();
  //delay(10);
  home_motor();

  //writeMotor(0);

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
    /*Serial.print(packet.strobe);
    Serial.print(" ");
    Serial.print(packet.sync);
    Serial.print(" ");
    Serial.println(packet.position);*/
    //DEBUG_PRINT(read_counter());

    if(packet.strobe) {
      digitalWrite(STROBE_PIN, HIGH);
    } else {
      digitalWrite(STROBE_PIN, LOW);
    }
    
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

  goToPosition(packet.position);
  
}  // end of loop
