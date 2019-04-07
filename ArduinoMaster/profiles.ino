#include <XModem.h>

// using as example: https://www.arduino.cc/en/Tutorial/ReadWrite

/*
  SD card read/write

 This example shows how to read and write data to and from an SD card file
 The circuit:
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 51
 ** MISO - pin 50
 ** CLK - pin 52
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
  unsigned long t = micros();
  int rx;
  
  do {
    rx = Serial.read();
    if(rx >= 0) {
      return rx;
    }
    if(micros() - t >= 1000) {
      msDelay--;
      t += 1000;
    }
  } while(msDelay > 0);
  
  return -1;
}

void sendChar(char sym) {
  Serial.write(sym);
}

File myFile;
int16_t result = -10;   // TODO: this goes away
uint32_t failures = 0;
uint32_t calls = 0;
uint32_t filesize = 0;              // decremented as file is written
char filename[13] = "LESLIE00.BIN"; // default filename
unsigned int index = 0;             // profile number

void profile_open() {
  //Serial.print("Opening "); Serial.print(filename);
  myFile = SD.open(filename);
  if(myFile) {
    //Serial.println(" ... Success");
    myFile.seek(HEADER_SIZE);
    row_init();
    count_output(filename + 6);   // pointer to profile number within filename
  } else {
    //Serial.println(" ... Failed");
  }     // TODO: output first row and hold for a while
}

void profile_open_by_index(unsigned int index_to_open) {
  myFile.close();
  index = index_to_open;
  sprintf(filename, "LESLIE%02d.BIN", index);
  profile_open();
}

XModem modem(recvChar, sendChar, dataHandler);

bool profile_download(char *fname, char *fsize) {
  myFile.close();
  
  strncpy(filename, fname, 12);

  if(SD.exists(filename)) {
    SD.remove(filename);
  }
  
  myFile = SD.open(filename, FILE_WRITE);
  if(!myFile) {
    failures++;
    return false;
  }
  
  filesize = atol(fsize);

  result = modem.receive();

  if(result) {
    
  } else {
    failures++;
    filesize = 0;
    myFile.close();
    SD.remove(filename);
  }

  index = extract_index(filename);
  Serial.println(index);
  profile_open();   // open profile that was just received

  return result;
}

bool dataHandler(unsigned long no, char* data, int size) {
  calls++;
  
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
    
    Serial.print("Wrote "); Serial.print(filename); Serial.print(" : "); Serial.println(size);
  }

  return true;
}

void profile_setup() {
  Serial.print("Initializing SD card...");

  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

  //profile_try_receive();

  Serial.println("Continuing startup...");
  
  profile_open();               // open default profile
}

int16_t profile_read(void* buf, uint16_t nbyte) {
  int16_t result;
  //Serial.print("Read from file... ");
  result = myFile.read(buf, nbyte);
  //Serial.println(result);
  if(result < nbyte) {                  // reached end of file (or error occurred), start over
    myFile.seek(HEADER_SIZE);
    result = myFile.read(buf, nbyte);
  }
  return result;
}

#define PROFILE_MIN 00
#define PROFILE_MAX 99

int profile_wrap(int n, int min, int max) {
  if(n > max) {
    n = min;
  } else if(n < min) {
    n = max;
  }
  return n;
}

int extract_index(char* filename) {
  char num[3];
  int n;
  num[0] = filename[6];
  num[1] = filename[7];
  num[2] = 0;
  n = atoi(num);
  return profile_wrap(n, PROFILE_MIN, PROFILE_MAX);
}

/* Returns index (0-99) unless an error occurred:
 * -1 = Could not open any file
 */
int profile_next(int direction) {
  int current;
  
  myFile.close();
  
  index = profile_wrap(index, PROFILE_MIN, PROFILE_MAX);
  current = index;
  
  do {
    index += direction;
    index = profile_wrap(index, PROFILE_MIN, PROFILE_MAX);
    sprintf(filename, "LESLIE%02d.BIN", index);
    profile_open();
    if(myFile) {
      return index;
    }
  } while(index != current);
  
  return -1;
}
