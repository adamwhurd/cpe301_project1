
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
 *  Temp sensor:
 *  left pin: goes to arduino board pin (I chose A7)
 *  Middle pin: +5v
 *  Right pin: ground
 *  
 *  Note that the register select, enable, and data pins can be set to other pin locations. Just be sure to make the variable 
 *  assignments match their locations.
 *  
 *  
 */
#include "DHT.h"
#include <LiquidCrystal.h>

#define DHT11_PIN A7  // set this value to whatever board pin your DHT data line is plugged into
DHT tempSensor(DHT11_PIN, DHT11);

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int reg_sel = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;

float temperature = 0.0;
LiquidCrystal lcd(reg_sel, en, d4, d5, d6, d7);

void setup() 
{
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  tempSensor.begin(); // start collecting temperature data
  
  displayTemperature(); // displays current temperature to the top line of the LCD
  
  // set the cursor to column 0, line 1(note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  
  // print the number of seconds since reset:
  char handPos = 'N';
  lcd.print("Hands good: ");
  lcd.print(handPos);
}

void loop()
{
  displayTemperature();
  delay(1000);
}

void displayTemperature()
{ // displays current temperature to the top line of the LCDv
  lcd.setCursor(0, 0);  // set cursor to column 0, row 0 (leftmost bit of top row)
  temperature = tempSensor.readTemperature(true); // This will read temperature as Fahrenheit. To read temp as Celsius, remove the "true" argument

  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print(" F");  // or "C" if temp is displayed in Celsius
}
