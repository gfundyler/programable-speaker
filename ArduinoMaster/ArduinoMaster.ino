
// 100 HZ INT in
// Data R Out
// Data L Out
// CLK    Out

typedef struct {
  uint16_t left, right;
  uint8_t dac[16];
} ProfileRow;

#define PATCH_UP 25
#define PATCH_DN 26

#define DISPLAY_BIT0 A11
#define DISPLAY_BIT1 A10
#define DISPLAY_BIT2 A9
#define DISPLAY_BIT3 A8
#define DISPLAY_BIT4 A15
#define DISPLAY_BIT5 A14
#define DISPLAY_BIT6 A13
#define DISPLAY_BIT7 A12

void setup() {
  
  Serial.begin(9600);
  
  // put your setup code here, to run once:
  pinMode(18, INPUT);  // 100 hz interrupt          
  pinMode(20, OUTPUT); // motor serial data R
  pinMode(21, OUTPUT); // motor serial data L
  pinMode(19, OUTPUT); // Motor serial clock

  pinMode(A8, OUTPUT); // Display Bit 0
  pinMode(A9, OUTPUT); // Display Bit 1
  pinMode(A10, OUTPUT); // Display Bit 2
  pinMode(A11, OUTPUT); // Display Bit 3
  pinMode(A12, OUTPUT); // Display Bit 4
  pinMode(A13, OUTPUT); // Display Bit 5
  pinMode(A14, OUTPUT); // Display Bit 6
  pinMode(A15, OUTPUT); // Display Bit 7

  pinMode(A0, INPUT); // Rotor Speed Pedal
  pinMode(A1, INPUT); // Effects Intensity Pedal
  pinMode(A2, INPUT); // Volume Pedal

  pinMode(PATCH_UP, INPUT);  // PATCH Up
  pinMode(PATCH_DN, INPUT);  // PATCH Down

  pinMode(27, OUTPUT); // DAC CS
  pinMode(38, OUTPUT); // DAC CLK
  pinMode(28, OUTPUT); // DAC LDAC

  digitalWrite(27, 1); //DAC CS
  digitalWrite(28, 1); //DAC LDAC
  digitalWrite(38, 0); // DAC CLK

  pinMode(49, OUTPUT); // DAC 2  Data
  pinMode(48, OUTPUT); // DAC 1  Data
  pinMode(47, OUTPUT); // DAC 4  Data
  pinMode(46, OUTPUT); // DAC 3  Data
  pinMode(45, OUTPUT); // DAC 6  Data
  pinMode(44, OUTPUT); // DAC 5  Data
  pinMode(43, OUTPUT); // DAC 8  Data
  pinMode(42, OUTPUT); // DAC 7  Data
  pinMode(37, OUTPUT); // DAC 14 Data
  pinMode(36, OUTPUT); // DAC 13 Data
  pinMode(35, OUTPUT); // DAC 16 Data
  pinMode(34, OUTPUT); // DAC 15 Data
  pinMode(33, OUTPUT); // DAC 10 Data
  pinMode(32, OUTPUT); // DAC 11 Data
  pinMode(31, OUTPUT); // DAC 9  Data
  pinMode(30, OUTPUT); // DAC 12 Data
  
  profile_setup();
  generate_row();   // generate the first row
}

//void multiShiftOut(int pins,int values){
  //
//}

void send_row(uint16_t *row);

// L, R, DAC 1-16
/*uint16_t row[18] = { // data to send out
    //0xCD3F, 0x222F,
    1, 4000,
    0x7800, 0x2B17, 0x200C, 0xFBFA,
    0x2743, 0xA795, 0x46AE, 0x9DF2,
    0xE4E9, 0x57B1, 0x9145, 0xD64D,
    0xD784, 0xAA0D, 0x1B70, 0x967D,
};*/

uint16_t DAC[16] = {
  0,0,0,0,
  0,0,0,0,
  0,0,0,0,
  0,0,0,0,
};

uint16_t formatPacket(byte start){
  // start as 8
  // shift left 4 places
  // add single one 1
  // and with 01110000
  uint16_t result = (uint16_t)start;
  result = result << 4;
  //111000000000000;
  result = 0X7000 | result;
  return  result;
}

//Lookup table 
// 16 positions
// 13 bytes
// 

// 0-1023
int getSpeedPedal(){
  //TODO: swap with fast read
  return analogRead(A0);
}

// 0-255
int getIntensity(){
  //TODO: swap with fast read
  return (byte)(analogRead(A1) << 2);       // FIXME: produces 0-4095 then truncates, not 0-255
}

// 0-255
int getVolumePedal(){
  //TODO: swap with fast read
  return (byte)(analogRead(A2) << 2);       // FIXME: produces 0-4095 then truncates, not 0-255
}


byte ModulateEFF(int volume,int  intensity){
  int result = (volume * intensity);
  result = result >> 8;
  return (byte) result;  
}
byte ModulateCenter(int volume,int intensity){
  int result = (volume * (255 - intensity));
  result = result >> 8;
  return (byte) result;
}

void assignDAC(){
  // Get pedal positions
  //byte pedalSpeed = getSpeedPedal();
  byte volume = getVolumePedal();
  byte intensity = getIntensity();

  // perform modulations 
  // modulatedC
  DAC[0] = formatPacket(ModulateCenter(volume, intensity));
  //modulatedEFF 
  DAC[1]= formatPacket(ModulateEFF(volume, intensity));
  
  int i = 2;
  for(i= 2; i< 16; i+=1){
  // 14 unknown bytes from lookup table
    DAC[i] = formatPacket(i);
  };
  for(i=2; i < 18; i+=1){
    // Copy Function instead
    //row[i] = DAC[i-2];      // GYF20190308 taken out temporarily
  }
  
}


#define MOTOR_CLOCK_PIN 2   // PORTD
#define DAC_CLOCK_PIN 7     // PORTD

#define CLOCK_BITS ((1 << MOTOR_CLOCK_PIN) | (1 << DAC_CLOCK_PIN))

/*void populate_row_u16(ProfileRow *data) {
  int i;
  row_u16[0] = data->left;
  row_u16[1] = data->right;
  for(i = 0; i < 16; i++) {
    row_u16[i+2] = data->dac[i];
  }
}*/

#define PROFILE_TRY_DELAY 100   // count of iterations through the loop

byte patch_up_latch = HIGH;
byte patch_dn_latch = HIGH;

void process_buttons() {
  byte pin;
  pin = digitalRead(PATCH_UP);
  if(pin == LOW && patch_up_latch == HIGH) {
    Serial.println("up");
    profile_next(1);
  }
  patch_up_latch = pin;
  
  pin = digitalRead(PATCH_DN);
  if(pin == LOW && patch_dn_latch == HIGH) {
    Serial.println("dn");
    profile_next(-1);
  }
  patch_dn_latch = pin;
}

#define UPDATE_PERIOD_MS    10      // how often to send a new row (in ms)
#define NEW_PROFILE_HOLDOFF 100     // on new profile, hold for this many UPDATE_PERIOD_MS intervals

unsigned long t = 0;
unsigned int profile_holdoff = NEW_PROFILE_HOLDOFF;
uint16_t row_u16[18];

void generate_row() {
  bool new_profile;
  new_profile = row_next(row_u16);
  if(new_profile) {
    profile_holdoff = NEW_PROFILE_HOLDOFF;
  }
}

void loop_real() {
    unsigned long t_new;

    t_new = millis();
    if(t_new - t >= UPDATE_PERIOD_MS) {
      t = t_new;
      //ProfileRow row;
      digitalWrite(27,0);
      //profile_read(&row, sizeof(ProfileRow));
      //populate_row_u16(&row);

      send_row(row_u16);
      int i;
      for(i = 0; i <10; i++){
        __asm__ volatile("nop\n\t");//1/16 us
      }
      PORTD &= ~CLOCK_BITS;                   // clock goes low

      if(profile_holdoff) {
        profile_holdoff--;
      } else {
        generate_row();
        //Serial.println(row_u16[0]);
      }
      digitalWrite(27,1);
      digitalWrite(28,0);
      digitalWrite(28,1);
  
      process_buttons();      
      row_calculate_step(getSpeedPedal());
      //Serial.println(getSpeedPedal());
    }

    command_byte(Serial.read());
}

void loop_test(){
  Serial.println("Speed");
  Serial.println(getSpeedPedal());
  Serial.println("Intensity");
  Serial.println(getIntensity());
  Serial.println("Volume");
  Serial.println(getVolumePedal());
  delay(5000);
}
void loop_clock_test(){
 digitalWrite(38,1);
 delay(100);
 digitalWrite(38,0);
 delay(100);
}
void loop(){
// loop_clock_test();
 loop_real();
}

void count_output(char *num) {
  char n;
  n = num[0];
  digitalWrite(DISPLAY_BIT0, n & 1);
  digitalWrite(DISPLAY_BIT1, n & 2);
  digitalWrite(DISPLAY_BIT2, n & 4);
  digitalWrite(DISPLAY_BIT3, n & 8);
  n = num[1];
  digitalWrite(DISPLAY_BIT4, n & 1);
  digitalWrite(DISPLAY_BIT5, n & 2);
  digitalWrite(DISPLAY_BIT6, n & 4);
  digitalWrite(DISPLAY_BIT7, n & 8);
}

//rol rotate to right by source bits
//lsl logical shift left

#define start(out, in)  asm volatile("lsl %0" : "=r" (in)  : "0" (in)); \
                        asm volatile("rol %0" : "=r" (out));

#define build(out, in)  asm volatile("lsl %0" : "=r" (in)  : "0" (in)); \
                        asm volatile("rol %0" : "=r" (out) : "0" (out));

static inline __attribute__((always_inline)) void send_row_bytes(uint8_t *byte) {
    register uint8_t array[18], pl, pd, pc;
    int i;
    
    pd = PORTD | CLOCK_BITS;
    
    array[ 0] = byte[ 0];
    array[ 1] = byte[ 2];
    array[ 2] = byte[ 4];
    array[ 3] = byte[ 6];
    array[ 4] = byte[ 8];
    array[ 5] = byte[10];
    array[ 6] = byte[12];
    array[ 7] = byte[14];
    array[ 8] = byte[16];
    array[ 9] = byte[18];
    array[10] = byte[20];
    array[11] = byte[22];
    array[12] = byte[24];
    array[13] = byte[26];
    array[14] = byte[28];
    array[15] = byte[30];
    array[16] = byte[32];
    array[17] = byte[34];
    
    for(i = 8; i > 0; i--) {
        start(pl, array[ 8]);
        build(pl, array[ 9]);
        build(pl, array[ 6]);
        build(pl, array[ 7]);
        build(pl, array[ 4]);
        build(pl, array[ 5]);
        build(pl, array[ 2]);
        build(pl, array[ 3]);
        
        PORTD = pd ^ CLOCK_BITS;
        pd &= ~CLOCK_BITS;
        pd >>= 2; // lose the two bits
        build(pd, array[ 1]); // replace them
        build(pd, array[ 0]);
        
        start(pc, array[13]);
        build(pc, array[10]);
        build(pc, array[12]);
        build(pc, array[11]);
        build(pc, array[16]);
        build(pc, array[17]);
        build(pc, array[14]);
        build(pc, array[15]);
        
        PORTL = pl;
        PORTD = pd;                         // clock goes high
        PORTC = pc;
    }

    for(i = 0; i <8; i++){
      __asm__ volatile("nop\n\t");//1/16 us
    }

    PORTD |= CLOCK_BITS;

}

void send_row(uint16_t *row) {
    send_row_bytes((uint8_t*)row + 1);      // send all MSB's
    send_row_bytes((uint8_t*)row);          // send all LSB's
}

/* 132 to first bit out clock high
    25 to clock low, 25 to clock high    x7
    80 to clock low, 25 to clock high
    25 to clock low, 25 to clock high    x7
    36 to clock low

    3.125 us/bit max (320 kHz)
    ~60 us total
*/

/*typedef union {
  struct {
    uint16_t position :13;
    uint16_t reserved :2;
    uint16_t strobe   :1;
  };
  uint16_t word;
} Position;*/

/*ProfileRow profile[] = {
  {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, },
  {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, },
};*/
