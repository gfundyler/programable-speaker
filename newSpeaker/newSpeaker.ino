int incomingByte = 0;   // for incoming serial data

// 0 CLK
// 1 Data // PORT 21
void setup() {
        pinMode(0,INPUT);
        pinMode(1,INPUT);
        Serial.begin(115200); // Serial 0 is computer
//        Serial1.begin(115200); // Serial 1 is ethernet cable
}

//https://www.arduino.cc/en/Hacking/PinMapping32u4
//readClock
//speedRead(PORTD,2);
//readData
//speedRead(PORTD,3);

//#define portInputRegister(P) ( (volatile uint8_t *)( pgm_read_word( port_to_input_PGM + (P))) )

#define wordEnhanced(addr)         \
(__extension__({                            \
    uint16_t __addr16 = (uint16_t)(addr);   \
    uint16_t __result;                      \
    __asm__                                 \
    (                                       \
        "lpm %A0, Z+"   "\n\t"              \
        "lpm %B0, Z"    "\n\t"              \
        : "=r" (__result), "=z" (__addr16)  \
        : "1" (__addr16)                    \
    );                                      \
    __result;                               \
}))

int speedRead(uint8_t port, uint8_t bit)
{
//  if (wordEnhanced( port_to_input_PGM + (port)) & bit) return HIGH;
  if (*portInputRegister(port) & bit) return HIGH;
  return LOW;
}

int digitalReadNoPWM(uint8_t pin)
{
  uint8_t bit = digitalPinToBitMask(pin);
  uint8_t port = digitalPinToPort(pin);
  if (*portInputRegister(port) & bit) return HIGH;
  return LOW;
}


uint16_t shiftIn16(uint16_t dataPin, uint16_t clockPin) {

  uint16_t value = 0;
  
  uint16_t i;
  
  for (i = 0; i < 16; ++i) {
    //Check the clock
//    while(!speedRead(PORTD,2)){
//    }  
//    value |= speedRead(PORTD,3) << (15 - i);
      while(!digitalRead(0)){
      }
      while(digitalRead(0)){
      }
      value |= digitalRead(1) << (15 - i);
  }
  
  return value;

}

void loop() {

        // send data only when you receive data:
                // read the incoming byte:
  //              incomingByte = Serial1.read();
       //         if(!digitalRead(0)){
                  incomingByte = shiftIn16(1, 0);
                  // say what you got:
                  Serial.println(incomingByte, HEX);
         //       }
         delay(1000);
}
 
