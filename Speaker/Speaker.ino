#define SERIAL_SPEED 115200


// http://stackabuse.com/speeding-up-arduino/
#define CLR(x,y) (x&=(~(1<<y)))
#define SET(x,y) (x|=(1<<y))

// Example
// This will set pin 8 to high
//SET(PORTB, 0);  


/*
 * Generally speaking, doing this sort of thing is not a good idea. Why not? Here are a few reasons: 
 * The code is much more difficult for you to debug and maintain, and is a lot harder for other people 
 * to understand. It only takes a few microseconds for the processor to execute code, but it might take 
 * hours  for you to figure out why it isn't working right and fix it! Your time is valuable, right? 
 * But the computer's time is very cheap, measured in the cost of the electricity you feed it. Usually 
 * it is much better to write code the most obvious way. 
 * 
 * The code is less portable. If you use digitalRead() and digitalWrite(), it is much easier to write 
 * code that will run on all of the Atmel microcontrollers, whereas the control and port registers 
 * can be different on each kind of microcontroller. 
 * It is a lot easier to cause unintentional malfunctions with direct port access. Notice how the line
 * 
 * DDRD = B11111110; above mentions that it must leave pin 0 as an input pin. Pin 0 is the receive line
 * (RX) on the serial port. It would be very easy to accidentally cause your serial port to stop working 
 * by changing pin 0 into an output pin! Now that would be very confusing when you suddenly are unable 
 * to receive serial data, wouldn't it? */


/* ARDUINO MODE */
#define MODE_LEFT_CHANNEL 1
#define MODE_RIGHT_CHANNEL 0
/* END ARDUINO MODE*/
#define board_mode MODE_LEFT_CHANNEL

/* SERIAL INPUT */
#define RECORD_SEPERATOR 124
#define UNIT_SEPERATOR 44
#define LEFT_MOTOR_POS 0
#define LEFT_STROBE_POS 1
#define RIGHT_MOTOR_POS 2
#define RIGHT_STROBE_POS 3

/* END SERIAL INPUT*/

/* COUNTER AND MOTOR */
#define CS0_PIN 13
#define SCK0_PIN  12
#define MISO0_PIN  11
#define MOSI0_PIN  10 /* input */
#define CS1_PIN  9
#define SCK1_PIN  8
#define MISO1_PIN  7
#define MOTOR_SATURATION 1024
#define MOTOR_RESET 6
#define IO_ENABLE 5
#define STROBE_PIN 2
/* END COUNTER AND MOTOR */

/*#define DEBUG*/
//define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(x)  Serial.println (x)
#define DEBUG_DELAY(x) delay(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_DELAY(x)
#endif

#define LOOP_CYCLE 80000
#define LOOP_TIMEOUT 10
bool should_strobe = 0;
bool has_comm = false;
byte _counter = 0;
byte loops_since_comm = 0; 

void strobe_blink(){
  if(should_strobe){
    digitalWrite(STROBE_PIN, HIGH);
    should_strobe = 0;
  } else {
    digitalWrite(STROBE_PIN, LOW);  
  }
}
void handle_timeout(){

  if(_counter > LOOP_CYCLE){
  if(has_comm){
   loops_since_comm = 0;
  }
  else {
    loops_since_comm +=1;
    if(loops_since_comm > LOOP_TIMEOUT){
      establishContact();
    }
  }
  has_comm = false;
     _counter = 0;
  } else {
    _counter +=1;
  }  

}

byte incomingBytes[100];
int index = 0;
int fieldIndex = 0;

int motor_message_pos = 0;
int strobe_message_pos = 0;

void initialize_from_board_mode(){
  /* TODO */
  if(board_mode==MODE_LEFT_CHANNEL){
    motor_message_pos = LEFT_MOTOR_POS;
    strobe_message_pos = LEFT_STROBE_POS;
  }
  if(board_mode==MODE_RIGHT_CHANNEL){
    motor_message_pos = RIGHT_MOTOR_POS;
    strobe_message_pos = RIGHT_STROBE_POS;
  }
}

void setup_speaker() {
  /*
    1)? Power On
    2)? Disable USB comms
    3)* Initialize I/O pin directions
    4)* Place PWM Processor in Reset
    5)* Place CS lines inactive (High)
    6)* Place Serial Clock lines Low
    7)* Wait 10 ms
    8)? Initialize Counter board (16 bit, 4x incremental, 2x filter, clear on index, enabled)
    9)? Clear counter status register
    10)? Command Open Loop Motor ($0400)
    11)? Wait for Index
    12)? Command Open Loop Motor ($0000)
    13)? Enable USB comms
    14)? Upon USB comms, enable servo loop
  */

  initialize_pins();
  initializeChipSelect();
  initializeClock();
  delay(10);
  reset_motor();
  home_motor();
}

bool led_state = false;
void led_toggle(){
  if(!led_state){
    digitalWrite(LED_BUILTIN, LOW);      
  } else {
    digitalWrite(LED_BUILTIN, HIGH);  
  } 
  led_state = !led_state;
}
void establishContact() {
    while(!Serial){
        ; // waiting for connection
    }
  while (Serial.available() <= 0) {
    Serial.println("SPEAKER");
    delay(300); 
    if(_counter > 4){
      led_toggle();
      _counter = 0;
    }
    _counter +=1;
  }
  _counter = 0;
}

// the setup function runs once when you press reset or power the board
void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    Serial.begin(SERIAL_SPEED);
    setup_speaker();
    establishContact();
    initialize_from_board_mode();
};

void wait_for_index(){
  byte x = read_str();
  byte mask = 0x05;
  while(!(x & mask)){
    delay(50);
    x = read_str();
  }  
 }

void clear_status_register(){
  const int WRITE_MDR0_CLEAR_REGISTER = 0x30;
  digitalWrite(CS0_PIN, LOW);
  shiftOut(MISO0_PIN, SCK0_PIN, MSBFIRST,WRITE_MDR0_CLEAR_REGISTER);
  digitalWrite(CS0_PIN, HIGH);   
}

void home_motor(){
    init_counter_clear();
    clear_status_register();
    writeMotor(2000);
    wait_for_index();
    writeMotor(0);
    DEBUG_PRINT("MOTOR HOMED");
}

void initializeIO() {
  digitalWrite(IO_ENABLE, HIGH);
}
void initializeClock() {
  digitalWrite(SCK0_PIN, LOW);
  digitalWrite(SCK1_PIN, LOW);
}
void reset_motor() {
  digitalWrite(MOTOR_RESET, LOW);
  delay(100);
  digitalWrite(MOTOR_RESET, HIGH);
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
  pinMode(MOTOR_RESET, OUTPUT);
  pinMode(IO_ENABLE, OUTPUT);
}

int target_value = 180;
int drive_value_multiplied = 0;

void update_target(int t) {
  if (target_value != t) {
    target_value = t;
    drive_value_multiplied = 0;
  }
}
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
void goToPosition(float x) {
  DEBUG_PRINT("(Command) Desired Position:");
  DEBUG_PRINT(x);
  float postition = read_counter();
  DEBUG_PRINT("(Actual) Position Value:");
  DEBUG_PRINT(postition);
  float error_value = getCommandOffset(x, postition);
  DEBUG_PRINT("(Error) Error In Degrees:");
  DEBUG_PRINT(error_value);
  const int gain = 25;
  float calculated_error_value = calculated_error(error_value);
  DEBUG_PRINT("(Error After Correction) Calculated Error Value In Degrees:");
  DEBUG_PRINT(error_value);
  drive_value_multiplied = calculated_error_value * gain;
  writeMotor(drive_value_multiplied);
}

float calculated_error(float error_value) {

  float result = error_value;
  if (error_value <= -180) {
    result += 360 ;
  }
  if (error_value >= 180) {
    result -= 360;
  }
  return result;
}

float getCommandOffset(float command_position, float actual) {
  return command_position - actual;
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
  byte result =  shiftIn(MOSI0_PIN, SCK0_PIN, LSBFIRST);
  digitalWrite(CS0_PIN, HIGH);
  return result;
}


float read_counter() {

  digitalWrite(CS0_PIN, LOW);
  shiftOut(MISO0_PIN/*dataPin*/, SCK0_PIN/*clockPin*/, MSBFIRST, 0x60);
  byte result1 =  shiftIn(MOSI0_PIN, SCK0_PIN, MSBFIRST);
  byte result2 =  shiftIn(MOSI0_PIN, SCK0_PIN, MSBFIRST);
  digitalWrite(CS0_PIN, HIGH);

  short s = (short) ((result1 << 8) | result2);
  if (s < 0) {
    s = 0x1756 + s;
  }
  return s / 16.59;
}
void handle_input() {
  if (Serial.available() > 0) {
    has_comm = true;
    // read the incoming byte:
    incomingBytes[index] = Serial.read();
    if (incomingBytes[index] == UNIT_SEPERATOR) {
      // end of field
      char data[index];
      memcpy(data, incomingBytes, index);
      //Clear
      memset(&incomingBytes[0], 0, sizeof(incomingBytes));
      if(fieldIndex==motor_message_pos){
        update_target((float)atoi(data));
      }
      if(fieldIndex==strobe_message_pos){
        should_strobe = atoi(data);
      }
      fieldIndex += 1;
      index = 0;
    } else if (incomingBytes[index] == RECORD_SEPERATOR) {
          fieldIndex = 0;
          index = 0;
        }
     else {if (incomingBytes[index] > 47 || incomingBytes[index] < 58){
            index += 1;
          }
     }
    // fieldIndex = 0 right
    // fieldIndex = 1 left
  }

}

// the loop function runs over and over again forever
void loop() {
  handle_timeout();
  handle_input();
  goToPosition(target_value);
  strobe_blink();
  //delay(1000);
}


