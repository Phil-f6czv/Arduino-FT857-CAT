/*
  Created:  2012.08.16
   Author:  James Buck, VE3BUX
      Web:  http://www.ve3bux.com
  If you use this code and / or the associated library, all I ask is that you "give a nod" by making a small mention
  in your code's documentation section. I've worked hard on getting this working, so a little recognition is appreciated. :)

  Modified : 2020.04.19 V1.04
  Author   : Ph Lonc, F6CZV
  Inclusion of a LCD 4 x 20 on I2C bus to display :
  - frequency and mode,
  - RX/TX status,
  - S Meter value,
  - VFO A / B status,
  - Keyer and Break-in status if mode is CW or CWR,
  - DSP configuration,
  - SPLIT status.
  The VFO status, Keyer, Break-in, DSP configutation and SPLIT status are read from the radio EEPROM.

  from V1.03 to V1.04
  the software was ported on ESP32 and some modifications were necessary. Some modifications independant of the screen management (LCD vs TFT)and platform
  were included in the Arduino version to a have a closer version of the libraries. 
  - GetSMeter is now a String function (was a char *) - conversion caused a run bug on ESP32
  - char frequency[8] is now frequency[9] (frequency display) to take into account the end of string character which caused a run exception on ESP32

LIMITATION OF LIABILITY :
 This source code is provided "as-is". It may contain bugs. 
Any damages resulting from its use is done under the sole responsibilty of the user/developper and beyond my responsibility.
  
  ------------------

  FT857D_controls_test

  This example sketch demonstrates how to use the FT-857D library to control a Yaesu FT-857D radio by calling various commands.
  Attach your radio's CAT interface to the following arduino pins to begin:

    Radio CAT port GND -> Arduino GND
    Radio CAT port TX  -> Arduino pin 11
    Radio CAT port RX  -> Arduino pin 10

  If once you compile and upload the code, you are unable to see the lock icon on the radio's LCD display, try reversing
  pins 2 and 3. It may be necessary to power down the radio and to reset the Arduino prior to powering the radio back on.

*/

#include <SoftwareSerial.h>
#include <Wire.h> // F6CZV
#include <LiquidCrystal_I2C.h> // LCD Library - F6CZV 
#include "FT857D.h"     // the file FT857D.h has a lot of documentation which I've added to make using the library easier

// LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display - F6CZV
LiquidCrystal_I2C lcd(0x3F,20,4);      // set the LCD address to 0x3F for a 20 chars and 4 lines - F6CZV

FT857D radio;           // define "radio" so that we may pass CAT commands
bool On_Air = false;
bool Split = false;
bool Keyer;
bool Break_In;
byte MeterConf;
bool DNF = false;
bool DNR = false;
bool DBF = false;
bool AGC = false;
String blank = "      ";
char * mode = "   ";
int dly = 500;            // delay for x milliseconds between commands

void setup() {

/*---- Working on a way to allow easier user input of I/O pins for software serial emulation
  ---- Right now, it is necessary to edit the FT857D.cpp file and to edit the line which states:
  ---- extern SoftwareSerial rigCat(11,10); // rx,tx
  ---- change the rx & tx pins to values which suit your application and save the FT857D.cpp file
*/

//  SoftwareSerial userSerial(11,10);  
//  rig.setSerial(userSerial);
  Serial.begin(4800);
  radio.begin(4800);    // as with Serial.begin(9600); we wish to start the software serial port
                        // so that we may control a radio via CAT commands
  lcd.init();           // initialize the lcd - F6CZV
  lcd.backlight();      // switch on the backlight - F6CZV
  lcd.setCursor(0,1); // (num colonne , num ligne) - F6CZV
  lcd.print("VFO"); // F6CZV
 
}

void loop() {         
 
/* perform a frequency sweep
   for (long freq=0; freq < 20; freq++){
    radio.setFreq(1400000 + freq * 500);
    displayFreqMode();
  }   */  
/*  radio.setFreq(1428163);  // set VFO frequency to xx */

  displaySMeter();
  displaySplit_status();
  displayRXTX();
  displayVFO();
  displayFreqMode();
  displayDSP();
  lcd.setCursor(12,3);
  lcd.print(blank);
  if ((mode == "CW ") || (mode == "CWR")) {displayCWConf();}
  delay(dly);
}

void displayDSP() {
radio.getAGC_DSP_Conf(AGC,DBF,DNR,DNF);
lcd.setCursor(0,3);
if (DBF) {lcd.print("DBF");} else {lcd.print("   ");}
lcd.setCursor(4,3);
if (DNF) {lcd.print("DNF");} else {lcd.print("   ");}
lcd.setCursor(8,3);
if (DNR) {lcd.print("DNR");} else {lcd.print("   ");}
}

void displayCWConf() {
radio.getCW_MTR_Conf(MeterConf,Keyer,Break_In); 
lcd.setCursor(12,3); // (num colonne , num ligne) 
  if (Keyer) {     
     lcd.print("KYR");}
     else {lcd.print("   ");}
  lcd.setCursor(16,3); // (num colonne , num ligne)
  if (Break_In) { 
    lcd.print("BK");}
     else {lcd.print("  ");}
}

void displayVFO() { // F6CZV
  lcd.setCursor(4,1); // (num colonne , num ligne) 
  lcd.print(radio.getVFO()); 
  }

  void displaySMeter() { // F6CZV
  lcd.setCursor(0,0);
  lcd.print("     ");
  lcd.setCursor(0,0);
  lcd.print(radio.getSMeter());
  }
  
  void displayRXTX() { // F6CZV
  On_Air = radio.chkTx();
  if (On_Air) {
    lcd.setCursor(18,0);
    lcd.print("Tx");}
  else
  {lcd.setCursor(18,0);
   lcd.print("Rx");}
  }

  void displaySplit_status() { // F6CZV
  Split = radio.getSPLIT_status();
  lcd.setCursor(13,0);
  if (Split) {
    lcd.print("SPL");}
  else
   {lcd.print("   ");}
  }
  
  void displayFreqMode() { // F6CZV
  char frequency[9];
  byte shift = 0;
  unsigned long tempfreq;
  int n;
  tempfreq = radio.getFreqMode();
  sprintf(frequency, "%ld", tempfreq);
  
  if (tempfreq < 10000000 & tempfreq >= 1000000) {
   shift = 1;
   }   
   if (tempfreq < 1000000 & tempfreq >= 100000) {
   shift = 2;
   }
   if (tempfreq < 100000) {
   shift = 3;
   }
   
  lcd.setCursor(7,2); // (num colonne , num ligne)
  if (shift != 3) {lcd.print(".");}
  
  for (n = 0; n < 3;n++) {
    lcd.setCursor(8+n,2); // (num colonne , num ligne)
    lcd.print(frequency[3+n-shift]); }
  
  lcd.setCursor(11,2); // (num colonne , num ligne)
  lcd.print(",");
  
  for (n = 0; n < 2;n++) {
    lcd.setCursor(12+n,2); // (num colonne , num ligne)
    lcd.print(frequency[6+n-shift]); }
  
  lcd.setCursor(4,2); // (num colonne , num ligne)
 
  for (n = 0; n < 3;n++) {
    lcd.setCursor(4+n,2); // (num colonne , num ligne)
    if ((n-shift)>= 0) {lcd.print(frequency[n-shift]);} 
    else
    {lcd.print(" ");}
  }
  lcd.setCursor(14,2); // (num colonne , num ligne)
  lcd.print(" kHz "); 
  shift=0;
  
  lcd.setCursor(0,2); // (num colonne , num ligne)
  mode = radio.getMode();
  lcd.print(mode);
  }

  

                  
  
