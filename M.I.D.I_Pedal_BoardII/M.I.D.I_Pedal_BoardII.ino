/* 
  MIDI Bass Pedal

  Author: Wilfredo Masso

  Date: 29/7/2019

  A MIDI Pedal with LCD display and auto on/off backlight.
  Two voices and a quick preset mode, let you to asign predefined
  parameters just presing & holding a momentary button and a keyboard note.

  Note: I do not use MIDI libraries in this project.

*/ 

/*
  The circuit:
  LCD RS pin to digital pin 12
  LCD Enable pin to digital pin 11
  LCD D4 pin to digital pin 5
  LCD D5 pin to digital pin 4
  LCD D6 pin to digital pin 3
  LCD D7 pin to digital pin 2
  LCD R/W pin to ground
  10K potentiometer:
  ends to +5V and ground
  wiper to LCD VO pin (pin 3)
  10K poterntiometer on pin A0
  10K poterntiometer on pin A1
  10K poterntiometer on pin A2
  10K poterntiometer on pin A3
  momentary button switch on pin A4 with pullup resistors activated
  CD 4052 1 of 4 decoder X 2  (see MIDI Bass Pedal schematic)
*/

// include the library code:
#include <LiquidCrystal.h>
#include "GMpatches.h"

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;  // will store last time L.C.D. was scrolled
unsigned long displayMillis = 0;
unsigned long displayLight = 0;

// constants won't change:
const long interval = 500; //used for timed interrupts

// pins were the control sensors are attached
const int sensorProgram = A0;
const int sensorChannel = A1;
const int sensorVelocity = A2;
const int sensorOctave = A3;

//Strings - variables storing text for show on the LCD
String stringOct;
String stringChn;
String stringVel;
String stringPgm;

// variables will change
int count;                    //for scolling L.C.D.
int sensorMin = 1023;        // minimum sensor value
int sensorMax = 0;           // maximum sensor value
int pgmSelect, chnSelect, octSelect, veloSelect, pgmSelect2, chnSelect2, octSelect2, veloSelect2;
byte sensors[] {sensorProgram, sensorChannel, sensorVelocity, sensorOctave};
byte sensorsValues[] {0, 0 , 0, 0};
byte lastSensorsValues [] {0, 0, 0, 0};
byte scaleDiv[] {16, 16, 16, 64};
int sentByte = -1; //this value ensures sending a Program Change after power on
//corresponding with the potentiometer position
int sentByte2 = -1;
int r0, r1, r2, r3;
int s0 = 6, s1 = 7 , s2 = 8, s3 = 9, inPin = 10, prgMixPin = A4;
byte currentNote[16] {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
byte lastNote[16] {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

// Bank Select message matrix
byte bankSel[] = {0xB0, 0x00, 0x00 , 0xB0, 0x20, 0x00};

// Program names matrix. put the 16 Programs you´ll need here
//in the order they must appear (see the GMPatches.h file for list)
char* progName[] = { GM032, GM033, GM034, GM035, GM036, GM037, GM038, GM039,
                     GM048, GM049, GM036, GM035, GM054, GM033, GM032, GM039
                   };
byte toggle = 1;

void setup() {
  //multiplexer out pins
  pinMode(s0, OUTPUT);
  pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT);
  pinMode(s3, OUTPUT);
  //backlight power pin
  pinMode(A5, OUTPUT);
  //multiplexer in pins
  pinMode(inPin, INPUT_PULLUP);
  pinMode(prgMixPin, INPUT_PULLUP);//quick menu button pin
  // initialize LCD and set up the number of columns and rows:
  lcd.begin(16 , 2);
  Serial.begin(31250);  //set to M.I.D.I.  baud rate
  lcd.home();
  digitalWrite(A5, HIGH);//turn backlight on
  printScreen("Arduino MIDI" , 2, 0);
  printScreen( "Bass Pedal" , 3, 1);
  delay(1500);
  while (millis() < 2500) {
    lcd.scrollDisplayLeft();
    delay(100);
  } lcd.clear();
  delay(100);
  printScreen( "Version 1.0" , 3, 0);
  printScreen( "Revision #2" , 3, 1);
  // srolling LCD during  two seconds
  delay(2000); //end of welcome screen
  //reset display
  lcd.clear();
}

void loop() {

  unsigned long currentMillis = millis();
  stringOct = ("Oct=");
  stringChn = ("Cha=");
  stringVel = ("Vel=");
  byte prgMix = digitalRead(prgMixPin);

  //sensors reading and LCD code *****************************************************************************+
  for (int y = 0; y < sizeof(sensors); y++) {
    int f = analogRead(sensors[y]);

    // apply the maping to the sensor reading
    f = map(f, sensorMin, sensorMax, 255, 0);

    // in case the sensor value is outside the range seen during calibration
    f = constrain(f, 0, 256);

    sensorsValues[y] = f / scaleDiv[y];

    if ( toggle == 1) {

      pgmSelect = (sensorsValues[0] );
      pgmSelect2 = pgmSelect;
      chnSelect = (sensorsValues[1]  + 1) ;
      chnSelect2 = chnSelect ;
      veloSelect = (sensorsValues[2]  * 4 + 70)  ;
      veloSelect2 = veloSelect ;
      octSelect = (sensorsValues[3]) ;
      octSelect2 = octSelect ;
      toggle  = 0;
    }

    if  (sensorsValues[y] < lastSensorsValues[y] ||
         sensorsValues[y] > lastSensorsValues[y]) {

      digitalWrite(A5, HIGH); // turn on backlight
      displayLight = currentMillis;

      lastSensorsValues[y] = sensorsValues[y]; //ensures LCD scroll until any potentiometer is movin again

      switch (y) {
        case 0:
          if (prgMix == 0 ) {
            pgmSelect2 = (sensorsValues[0] );
            printScreen( progName[pgmSelect2] , 0, 0);
          }
          else {
            pgmSelect = (sensorsValues[0] );
            printScreen( progName[pgmSelect] , 0, 0);
          }
          count = 0;  // this will reset the LCD´s text position if it was previously scrolled
          break;

        case 1:
          panic();
          if (prgMix == 0 ) {
            chnSelect2 = (sensorsValues[1] + 1);
            printScreen((stringChn + chnSelect2 + " " ) , 0, 1);
            sentByte2 = -1;
          }
          else {
            chnSelect = (sensorsValues[1] + 1);
            printScreen((stringChn + chnSelect + " " ) , 0, 1);
            sentByte = -1;
          }

          break;

        case 2:
          if (prgMix == 0 ) {
            veloSelect2 = (sensorsValues[2]  * 4 + 70)  ;
            printScreen((stringVel + veloSelect2 + " "), 8, 1);
          }
          else {
            veloSelect = (sensorsValues[2]  * 4 + 70)  ;
            printScreen((stringVel + veloSelect + " "), 8, 1);
          }
          displayMillis = currentMillis;
          beep();
          break;

        case 3:
          panic();
          if (prgMix == 0 ) {
            octSelect2 = (sensorsValues[3]) ;
            printScreen((stringOct + octSelect2 + "  "), 8, 1);
          }
          else {
            octSelect = (sensorsValues[3]) ;
            printScreen((stringOct + octSelect + "  "), 8, 1);
          }
          beep();
          break;
      }
    }

    //send characters to the LCD
    if (prgMix == 1) {
      if (count < 5)
        printScreen( progName[pgmSelect], 0, 0);
      printScreen(stringChn + chnSelect + " " , 0, 1);

      if (currentMillis - displayMillis <= interval * 4)
        printScreen((stringVel + veloSelect + " "), 8, 1);
      else
        printScreen(stringOct + octSelect + "  ", 8, 1);
    }

    if (prgMix == 0) {
      displayLight  = currentMillis;
      digitalWrite(A5, HIGH);
      printScreen( progName[pgmSelect2], 0, 0);
      printScreen(stringChn + chnSelect2 + " " , 0, 1);

      if (currentMillis - displayMillis <= interval * 4)
        printScreen((stringVel + veloSelect2 + " "), 8, 1);
      else
        printScreen(stringOct + octSelect2 + "  ", 8, 1);
      count = 0;
    }
    if (chnSelect2 < 3) {
      lcd.print("s");
    }
    else if (chnSelect == chnSelect2)
      lcd.print("@");
    else lcd.print("d");
  }
  if (currentMillis - displayLight >= interval * 10)
    digitalWrite(A5, LOW);// turn off backlight, comment if you want backlight always on
  //************************************************************************


  //multiplexer code
  //**************************************************************************
  for ( int val = 0; val < 16 ; val++) {

    r0 = bitRead(val, 0);
    r1 = bitRead(val, 1);
    r2 = bitRead(val, 2);
    r3 = bitRead(val, 3);

    digitalWrite(s0, r0);
    digitalWrite(s1, r1);
    digitalWrite(s2, r2);
    digitalWrite(s3, r3);
    //delay(2);
    currentNote[val] = digitalRead(inPin);

    switch (prgMix) {

      case 1:    // play mode

        if (currentNote[val] != lastNote[val]) { //a note event ocurred
          lastNote[val] = currentNote[val];
          if (currentNote[val] == 0) {          //at least one key is pressed
            noteSend(0X90 | chnSelect - 1, 0x15 + (octSelect * 12) + val , veloSelect);
            if (chnSelect2 > 2 )
              noteSend(0X90 | chnSelect2 - 1, 0x15 + (octSelect2 * 12) + val , veloSelect2);
          }

          else {
            noteSend(0x80 | chnSelect - 1, 0x15 + (octSelect * 12) + val, veloSelect);
            if (chnSelect2 > 2)
              noteSend(0X80 | chnSelect2 - 1, 0x15 + (octSelect2 * 12) + val , veloSelect2);
          }
        }
        break;

      case 0: //menu mode

        if (currentNote[val] != lastNote[val]) { //a note event ocurred
          lastNote[val] = currentNote[val];
          if (currentNote[val] == 0) {
            displayMillis = currentMillis;
            switch (val) {
              case 0:
                chnSelect2 = 5;
                octSelect2 = 2;
                pgmSelect2 = 9;
                sentByte2 = -1;

                break;
              case 1:
                chnSelect2 = 2;
                beep();
                break;
              case 2:
                chnSelect2 = 5;
                octSelect2 = 2;
                pgmSelect2 = 12;
                sentByte2 = -1;
                break;
              case 3:
                beep();
                break;
              case 4:
                break;
              case 5:
                break;
              case 6:
                break;
              case 7:
                break;
              case 8:
                break;
              case 9:
                break;
              case 10:
                break;

            }
          }
          else {
            noteSend(0x80 | chnSelect - 1, 0x15 + (octSelect * 12) + val, veloSelect);
            if (chnSelect2 > 2)
              noteSend(0X80 | chnSelect2 - 1, 0x15 + (octSelect2 * 12) + val , veloSelect2);
          }
        }
    }
  }
  //**************************************************************************

  if (currentMillis - previousMillis >= interval && count < 14 ) {
    // save the last time you scroll the L.C.D.
    previousMillis = currentMillis;
    count++;

    // Scrolling the LCD

    if ( count > 10) {
      stringPgm = (progName[pgmSelect]);
      String sub2 = stringPgm.substring(count - 9);
      lcd.clear();
      printScreen(sub2 , 0, 0);//hide numbers and show the full Program names
    }
  }

  //call a method
  if (sentByte != pgmSelect) {
    bnkSelect(progName[pgmSelect], chnSelect - 1);
    sentByte = pgmSelect;
  }
  if (sentByte2 != pgmSelect2) {
    bnkSelect(progName[pgmSelect2], chnSelect2 - 1);
    sentByte2 = pgmSelect2;
  }
}

// print text strings onto the LCD
void printScreen ( String text , int charN, int line) {
  // set the cursor to the top left
  lcd.setCursor(charN, line);
  lcd.print(text);
}

void beep () {
  tone(13, 1800, 20);
}

void noteSend(int command, int note, int velocity) {

  Serial.write(command);//send note on or note off command
  Serial.write(note);//send pitch data
  Serial.write(velocity);//send velocity data
}

//send the Bank Sel and Program Change message
void bnkSelect( String pgm, byte chn ) {
  for (int i = 0; i < sizeof(bankSel); i++) {
    Serial.write(bankSel[i]);
  }
  // extract and send the numbers of the Program name
  String sub = pgm.substring(1, 4) ;
  Serial.write(0xC0 | chn);
  Serial.write(sub.toInt());
  beep(); //end of message
}

void panic() {                  // All notes off
  for (int m = 0; m < 16; m++) {
    noteSend(0x80 | chnSelect - 1, 0x15 + (octSelect * 12) + m, 0x00);
    noteSend(0x80 | chnSelect2 - 1, 0x15 + (octSelect2 * 12) + m, 0x00);
  }
}
