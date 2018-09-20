// 100 HZ INT in
// Data R Out
// Data L Out
// CLK    Out

//A0 Rotor speed
//A1 effects intensity
//A2 Volume
  
void setup() {
  // put your setup code here, to run once:
  pinMode(0, INPUT);  // 100 hz interrupt          
  pinMode(1, OUTPUT); // motor serial data R
  pinMode(2, OUTPUT); // motor serial data L
  pinMode(3, OUTPUT); // Motor serial clock

  pinMode(14, OUTPUT); // Display Bit 0
  pinMode(15, OUTPUT); // Display Bit 1
  pinMode(16, OUTPUT); // Display Bit 2
  pinMode(17, OUTPUT); // Display Bit 3
  pinMode(18, OUTPUT); // Display Bit 4
  pinMode(19, OUTPUT); // Display Bit 5
  pinMode(20, OUTPUT); // Display Bit 6
  pinMode(21, OUTPUT); // Display Bit 7

  pinMode(25, INPUT);  // PATCH Up
  pinMode(26, INPUT);  // PATCH Down

  pinMode(30, OUTPUT); // DAC CS
  pinMode(31, OUTPUT); // DAC CLK
  pinMode(28, OUTPUT); // DAC LDAC


  pinMode(48, OUTPUT); // Future DAC 
  pinMode(49, OUTPUT);
  pinMode(46, OUTPUT);
  pinMode(47, OUTPUT);
  pinMode(44, OUTPUT);
  pinMode(45, OUTPUT);
  pinMode(42, OUTPUT);
  pinMode(43, OUTPUT);
  pinMode(40, OUTPUT);
  pinMode(41, OUTPUT);
  pinMode(38, OUTPUT);
  pinMode(39, OUTPUT);
  pinMode(36, OUTPUT);
  pinMode(37, OUTPUT);
  pinMode(34, OUTPUT);
  pinMode(35, OUTPUT);

}

void multiShiftOut(int pins,int values){
  //
}

void loop() {
  // put your main code here, to run repeatedly:
  // Falling edge
  
  //Most significant bit first

  byte stateR = 0;
  byte stateL = 0;
  int i = 0;
  int toSendR = 0xFAF5;
  int toSendL = 0x0A05;
  bool sendL[16];
  bool sendR[16];
  
  //clock rising edge
  digitalWrite(1, LOW); //R  
  digitalWrite(2, LOW); //L  
  digitalWrite(3, HIGH); //Clock

  // Start Pream
  digitalWrite(3, LOW); //Clock
  for(i=15;i>=0;i-=1){
    sendL[i]= bitRead( toSendR, i);
    sendR[i]= bitRead( toSendL, i);
  }
  
  //Send Pack
  for(i=15;i>=0;i-=1){
    digitalWrite(1, sendR[i]); //R
    digitalWrite(2, sendL[i]); //L
    digitalWrite(3, LOW); //Clock
    digitalWrite(3, HIGH); //Clock
  }
  

  delay(10);

}
