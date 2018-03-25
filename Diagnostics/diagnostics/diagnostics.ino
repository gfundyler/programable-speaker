#define SERIAL_SPEED 9600


/* SERIAL INPUT */
#define RECORD_SEPERATOR 124
#define UNIT_SEPERATOR 44

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
/* END COUNTER AND MOTOR */




byte incomingBytes[100];
int index = 0;
int fieldIndex = 0;


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

// the setup function runs once when you press reset or power the board
void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    Serial.begin(SERIAL_SPEED);
    setup_speaker();
};

/* TO BE ADDED TO SPEAKER */


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

void home_motor(){
    Serial.println("Homing Motor");
    delay(2000);
    init_counter_clear();
    clear_status_register();
    writeMotor(2000);
    wait_for_index();
    writeMotor(0);    
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

  /* OUTPUT */
  pinMode(CS0_PIN, OUTPUT);
  pinMode(SCK0_PIN, OUTPUT);
  pinMode(MISO0_PIN, OUTPUT);
  pinMode(CS1_PIN, OUTPUT);
  pinMode(SCK1_PIN, OUTPUT);
  pinMode(MISO1_PIN, OUTPUT);
  pinMode(MOTOR_RESET, OUTPUT);
  pinMode(IO_ENABLE, OUTPUT);

  /* INPUT */
  pinMode(MOSI0_PIN, INPUT);

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
  Serial.println("(PWM) Motor Drive:");
  Serial.println(value);

  digitalWrite(CS1_PIN, LOW);
  shiftOut(MISO1_PIN/*dataPin*/, SCK1_PIN/*clockPin*/, MSBFIRST, highByte(value));
  shiftOut(MISO1_PIN/*dataPin*/, SCK1_PIN/*clockPin*/, MSBFIRST, lowByte(value));
  digitalWrite(CS1_PIN, HIGH);
};
void goToPosition(float x) {
  Serial.println("(Command) Desired Position:");
  Serial.println(x);
  //init_counter_clear();
  float postition = read_counter();
  Serial.println("(Actual) Position Value:");
  Serial.println(postition);
  float error_value = getCommandOffset(x, postition);
  Serial.println("(Error) Error In Degrees:");
  Serial.println(error_value);
  const int gain = 25;/*25*/
  float calculated_error_value = calculated_error(error_value);
  Serial.println("(Error After Correction) Calculated Error Value In Degrees:");
  Serial.println(error_value);
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
  Serial.println("Raw Encoder");
  Serial.println(s);
  
  return s / 16.59;
}
void handle_input() {
  if (Serial.available() > 0) {
    // read the incoming byte:
    incomingBytes[index] = Serial.read();
    if (incomingBytes[index] == UNIT_SEPERATOR) {
      // end of field
      char data[index];
      memcpy(data, incomingBytes, index);
      //Clear
      memset(&incomingBytes[0], 0, sizeof(incomingBytes));
      if(fieldIndex==0){
        update_target((float)atoi(data));
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
  }

}

// the loop function runs over and over again forever
void loop() {
  handle_input();
  goToPosition(target_value);
}


