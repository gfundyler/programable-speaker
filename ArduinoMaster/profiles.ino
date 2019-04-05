#include <YModem.h>

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
    /*if(size == 0) {                             // TODO: implement this in ymodem library
      failures++;
      filesize = 0;
      myFile.close();
      SD.remove(filename);
    } else*/ if(size < filesize) {
      myFile.write(data, size);
      filesize -= size;
    } else {
      myFile.write(data, filesize);
      filesize = 0;
      myFile.close();
      
      Serial.print("Wrote "); Serial.print(filename); Serial.print(" : "); Serial.println(sizestring);
      
      index = extract_index(filename);
      Serial.println(index);
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

  // open the file. note that only one file can be open at a time,
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
  }

  // re-open the file for reading:
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
  }

  /*if(Serial) {
    result = modem.receive();
    Serial.println("");
    Serial.print("Tried "); Serial.println(filename);
    Serial.println(filesize);
    Serial.println(failures);
    Serial.println(result);
    Serial.println("Continuing startup...");
  }*/

  profile_open();               // open default profile
}

void profile_try_receive() {
  modem.receiveCrcPoll();
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
