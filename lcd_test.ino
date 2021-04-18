
/*
 * https://www.arduino.cc/en/Reference/LiquidCrystal
 * 
 * This program is a way to test the LCD screen. Wire the LCD as follows, for left to right:
 *
 *  VSS: ground
 *  VDD: +5v
 *  V0: middle pin of potentiometer - controls contrast
 *  RS: pin 12  - register select
 *  RW: ground - this is read/write
 *  E: pin 11 - this is enable
 *  D0 through D3: disconnected
 *  D4: 5
 *  D5: 4
 *  D6: 3
 *  D7: 2
 *  A: 330 ohm resistor, then to +5v - this is the anode
 *  K: ground - this is the cathode
 *  
 *  
 *  Note that the register select, enable, and data pins can be set to other pin locations. Just be sure to make the variable 
 *  assignments match their locations.
 *  
 *  
 */



#include <LiquidCrystal.h>

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int reg_sel = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(reg_sel, en, d4, d5, d6, d7);

void setup() 
{
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Here's how we can display stuff
  int temp = 70;
  lcd.print("Temp: ");
  lcd.print(temp);
  lcd.print(" F");
  
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  
  
  // print the number of seconds since reset:
  char handPos = 'N';
  lcd.print("Hands good: ");
  lcd.print(handPos);
}

void loop()
{
  delay(1000);
}
