
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

byte[] ReceiveSPI(){
  byte[3] result;
  
  Read SPDR // Get High Byte
  Clear SPIF // We will poll this for second byte
  CLI // Do not allow second byte to create interrupt

  //Polling Loop
  while(/*SPSR bit 7 == 0 || pollingcount > maxpollingtries*/ ){
  
  }
  if(pollingcount > maxpollingtries){
    // byte3 set to error
  } else {
    Clear SPIF
    Read SPDR, SEI
    RETI
  }
  return result;
  }
}

void loop() {
  // put your main code here, to run repeatedly:

  //read the encoder
  //calc drive value
  //write motor
  
}
