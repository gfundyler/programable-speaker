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

#define HEADER_SIZE 256   // header content TBD

int recvChar(int msDelay) {
  int cnt = 0;
  
  if(Serial.available() > 0)
    return Serial.read();
  
  while(cnt < msDelay) {
    delay(1);
    if(Serial.available() > 0)
      return Serial.read();
    cnt++;
  }
  
  return -1;
}

void sendChar(char sym) {
  Serial.write(sym);
}

File myFile;
int16_t result = -10;   // TODO: this goes away
uint32_t failures = 0;
uint32_t filesize = 0;  // decremented as file is written
char filename[13] = "LESLIE00.BIN"; // default filename
int index = 0;                      // profile number

void profile_open() {
  myFile = SD.open(filename);
  if(myFile) {
    myFile.seek(HEADER_SIZE);
    row_init();
  } 
}

bool dataHandler(unsigned long no, char* data, int size) {
  //char *filename, *sizestring;
  char *sizestring;
  //char *size_string, *delim;
  if(no == 0) {    // header containing filename and size
    strncpy(filename, data, 12);  // 8.3 filename
    //size_string = data + strlen(data) + 1;
    //delim = strstr(size_string, " ");   // size, date, etc. are delimited with spaces
    //if(delim) {
    //  *delim = NULL;                    // null-terminate the file size
    //}

    //filename = data;
    
    myFile.close();

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
    if(size == 0) {                             // TODO: implement this in ymodem library
      failures++;
      filesize = 0;
      myFile.close();
      SD.remove(filename);
    } else if(size < filesize) {
      myFile.write(data, size);
      filesize -= size;
    } else {
      myFile.write(data, filesize);
      filesize = 0;
      myFile.close();
      
      index = extract_index(filename);
      profile_open();   // open profile that was just received
    }
  }
  return true;
}

YModem modem(recvChar, sendChar, dataHandler);

void profile_setup() {
  Serial.print("Initializing SD card...");

  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
  
  profile_open();               // open default profile
}

void profile_try_receive() {
  modem.receiveCrcPoll();
}

int16_t profile_read(void* buf, uint16_t nbyte) {
  int16_t result;
  result = myFile.read(buf, nbyte);
  if(result < nbyte) {                  // reached end of file (or error occurred), start over
    myFile.seek(HEADER_SIZE);
    result = myFile.read(buf, nbyte);
  }
  return result;
}

#define PROFILE_MIN 00
#define PROFILE_MAX 99

int bound(int n, int min, int max) {
  if(n > max) {
    n = min;
  } else if(n < min) {
    n = max;
  }
}

int extract_index(char* filename) {
  char index[3];
  int n;
  index[0] = filename[6];
  index[1] = filename[7];
  index[2] = 0;
  n = atoi(index);
  return bound(n, PROFILE_MIN, PROFILE_MAX);
}

/* Returns index (0-99) unless an error occurred:
 * -1 = Invalid direction
 * -2 = Could not open any file
 */
int profile_next(int current, int direction) {
  if(direction == 0) {
    return -1;
  }
  
  direction = bound(direction, -1, 1);
  
  myFile.close();
  
  current = bound( current, PROFILE_MIN, PROFILE_MAX);
  index = current;
  
  do {
    index += direction;
    index = bound(index, PROFILE_MIN, PROFILE_MAX);    
    sprintf(filename, "LESLIE%02d.BIN", index);
    profile_open();
    if(myFile) {
      return 0;
    }
  } while(index != current);
  
  return index;
}