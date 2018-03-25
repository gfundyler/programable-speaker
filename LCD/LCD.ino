#include <LiquidCrystal.h>
String version_info = "Version 0.0.9";

#define SERIAL_SPEED 9600
/* SERIAL INPUT */
#define RECORD_SEPERATOR 124
#define UNIT_SEPERATOR 44
/* END SERIAL INPUT*/

bool has_handshake = 1;
/* LCD KEYPAD START */
int lcd_key     = 0;
int adc_key_in  = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

/* LCD KEYPAD END */

/*#define DEBUG*/

#ifdef DEBUG
#define DEBUG_PRINT(x)  Serial.println (x)
#define DEBUG_DELAY(x) delay(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_DELAY(x)
#endif

byte incomingBytes[100];
int index = 0;
int fieldIndex = 0;
int output;
char progress[]="-|/";
byte process_number = 0;
int progress_update = 0;

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);           // select the pins used on the LCD panel
void setup_display() {
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("-ReadyForConnect");
  lcd.setCursor(0, 1);
  lcd.print(version_info);
}
char get_progress(){
  if(progress_update > 35){
    progress_update = 0;
    process_number +=1;
    if(process_number > 2){
      process_number = 0;
    }
  }
  progress_update+=1;
 return progress[process_number];
}
void update_lcd_serial_progress(){
        lcd.setCursor(0, 0);
        lcd.print(get_progress());
}
void update_lcd_serial_connected(){
        lcd.setCursor(0, 0);
        lcd.print("Serial Connected");  
        lcd.setCursor(0, 1);
        lcd.print("- Handshaking -"); }

void update_lcd_serial_handshake(){
        lcd.setCursor(0, 0);
        lcd.print("Handshake Done...");  
        lcd.setCursor(0, 1);
        lcd.print("                 "); }

 void initialize_pins() {
    pinMode(A1, INPUT);
 }
// the setup function runs once when you press reset or power the board

void setup() {
    initialize_pins();
    setup_display();
    Serial.begin(SERIAL_SPEED);
    while(!Serial){
        update_lcd_serial_progress();
        ; // waiting for connection
    }
    update_lcd_serial_connected();
    update_lcd_serial_handshake();    Serial.println("LCD");

};

void establishContact() {
  while (Serial.available() <= 0) {
    delay(300); 
  }
}
/*DISPLAY REGION*/

int read_expression(){
  return analogRead(A1);
}

int read_LCD_buttons()
{
  adc_key_in = analogRead(0);      // read the value from the sensor
  // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
  // we add approx 50 to those values and check to see if we are close
  if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
  // For V1.1 us this threshold
  if (adc_key_in < 50)   return btnRIGHT;
  if (adc_key_in < 250)  return btnUP;
  if (adc_key_in < 350)  return btnDOWN;
  if (adc_key_in < 550)  return btnLEFT;
  if (adc_key_in < 850)  return btnSELECT;

  // For V1.0 comment the other threshold and use the one below:
  /*
    if (adc_key_in < 50)   return btnRIGHT;
    if (adc_key_in < 195)  return btnUP;
    if (adc_key_in < 380)  return btnDOWN;
    if (adc_key_in < 555)  return btnLEFT;
    if (adc_key_in < 790)  return btnSELECT;
  */


  return btnNONE;  // when all others fail, return this...
}

void update_display(byte row, char text[]) {
  lcd.setCursor(0, row);
  lcd.print(text);  
}
/*END DISPLAY REGION*/

void handle_input() {
  if (Serial.available() > 0) {
    // read the incoming byte:
    incomingBytes[index] = Serial.read();
    if (incomingBytes[index] == UNIT_SEPERATOR) {
      // end of field
      char data[index];
      memcpy(data, incomingBytes, index);
      //Clear
      memset(&incomingBytes[0], 0, sizeof(incomingBytes));
      update_display(fieldIndex,data);
      fieldIndex += 1;
      index = 0;
    } else if (incomingBytes[index] == RECORD_SEPERATOR) {
          fieldIndex = 0;
          index = 0;
        }
     else {if (incomingBytes[index] > 47 || incomingBytes[index] < 58){
            index += 1;
          }
     }
  }
}
void loop_display() {
     String outvalue = "";
     outvalue += read_LCD_buttons();
     outvalue += ",";
     outvalue += read_expression();
     Serial.println(outvalue);
}
// the loop function runs over and over again forever
void loop() {
    DEBUG_DELAY(5000);
    handle_input();
    loop_display();
}


