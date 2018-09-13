// 100 HZ INT in
// Data R Out
// Data L Out
// CLK    Out


  
void setup() {
  // put your setup code here, to run once:
  pinMode(0, INPUT);          
  pinMode(1, OUTPUT);          
  pinMode(2, OUTPUT);          
  pinMode(3, OUTPUT);          
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
