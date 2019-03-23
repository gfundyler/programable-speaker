#include <YModem.h>

// using as example: https://www.arduino.cc/en/Tutorial/ReadWrite

/*
  SD card read/write

 This example shows how to read and write data to and from an SD card file
 The circuit:
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4 (for MKRZero SD: SDCARD_SS_PIN)

 created   Nov 2010
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe

 This example code is in the public domain.

 */

#include <SPI.h>
#include <SD.h>

File myFile;

int recvChar(int msDelay) {
  int cnt = 0;
  while(cnt < msDelay) {
    if(Serial.available() > 0)
      return Serial.read();
    delay(1);
    cnt++;
  }
  return -1;
}

void sendChar(char sym) {
  Serial.write(sym);
}

int16_t result = -10;   // TODO: this goes away
uint32_t filesize = 0;  // decremented as file is written
//char filename[100] = {0};
uint32_t failures = 0;

bool dataHandler(unsigned long no, char* data, int size) {
  char *filename, *sizestring;
  //char *size_string, *delim;
  if(no == 0) {    // header containing filename and size
    //strcpy(filename, data);
    //size_string = data + strlen(data) + 1;
    //delim = strstr(size_string, " ");   // size, date, etc. are delimited with spaces
    //if(delim) {
    //  *delim = NULL;                    // null-terminate the file size
    //}

    filename = data;
    
    if(myFile) {
      myFile.close();
    }

    if(SD.exists(filename)) {
      SD.remove(filename);
    }
    
    myFile = SD.open(filename, FILE_WRITE);
    if(!myFile) {
      failures++;
      return false;
    }
    
    sizestring = data + strlen(data) + 1;   // size comes after filename
    filesize = atol(sizestring);
  } else {
    if(!myFile) {
      failures++;
      return false;
    }
    if(size < filesize) {
      myFile.write(data, size);
      filesize -= size;
    } else {
      myFile.write(data, filesize);
      filesize = 0;
      myFile.close();
    }
  }
  return true;
}

YModem modem(recvChar, sendChar, dataHandler);

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }


  Serial.print("Initializing SD card...");

  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

  //delay(2000);
  //while(1) {
    result = modem.receive();
  //}

  /*while(1) {
    if(Serial.available() > 0)
      Serial.println(Serial.read());
    else
      delay(1);
  }*/

  /*// open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open("test.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to test.txt...");
    myFile.println("testing 1, 2, 3.");
    // close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }*/

  /*// re-open the file for reading:
  myFile = SD.open("test.txt");
  if (myFile) {
    Serial.println("test.txt:");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }*/

  do {
    //while(!Serial) {}
    //Serial.println(filename);
    Serial.print(Serial.available());
    Serial.print(" ");
    Serial.println(Serial.read());
    delay(200);
  } while(1);


}

void loop() {
  // nothing happens after setup
}
