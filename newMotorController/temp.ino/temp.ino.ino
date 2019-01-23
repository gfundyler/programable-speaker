/*
See https://www.arduino.cc/en/Reference/SPI
 */

void setup() {
  // put your setup code here, to run once:
SPI.beginTransaction (SPISettings (2000000, MSBFIRST, SPI_MODE0));
//digitalWrite (SS, LOW);        // assert Slave Select
byte foo = SPI.transfer (42);  // do a transfer
digitalWrite (SS, HIGH);       // de-assert Slave Select
SPI.endTransaction ();         // transaction 

}

void loop() {
  // put your main code here, to run repeatedly:

}
