
/*
 * HARDWARE TEST PROGRAM
 * 
 * This program is a way to test the various hardware components to verify functionality.
 * It is not in any way meant to be reflective of the project.
 * 
 * 
 * MASTER PIN-OUT: ARDUINO CONNECTIONS
 * note that these are not set in stone; many of these pins can be re-assigned.
 * Starting from pin 13 and going clockwise:
 * 
 * 12 - RS on LCD 
 * 11 - E on LCD 
 * 
 * 8 - middle pin of transistor for fan motor via a 220 ohm resistor
 * 
 * 5 - D4 on LCD 
 * 4 - D5 on LCD
 * 3 - D6 on LCD
 * 2 - D7 on LCD
 * 
 * SDA - 20 - SDA pin on DS 1307 RTC module
 * SCL - 21 - SCL pin on DS 1307 RTC module
 * 
 * 
 * 36 - Servo motor pin (orange side)
 * 
 * 50 - trigger pin on ultrasonic
 * 52 - echo pin on ultrasonic
 * 
 * A15 - button input
 * 
 * A11 - yellow LED output via 330 ohm resistor
 * A10 - green LED output via 330 ohm resistor
 * 
 * A7 - DHT11 input (leftmost pin, when facing the blue grill)
 * 
 * 5V and GND go to the rails.
 * 
 * 
 * 
 * 
 * LCD screen. Wire the LCD as follows, for left to right:
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
 *  Servo motor: 
 *  Brown: ground
 *  Red: +5v
 *  Orange: board pin
 *  
 *  Fan motor: pos and neg must be seperated by a diode and controlled by a transistor.
 *  
 */
#include "DHT.h"  // temp sensor
#include <LiquidCrystal.h>  // LCD
#include <Servo.h>  // servo motor

// for RTC:
#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 rtcModule;

#define DHT11_PIN A7  // set this value to whatever board pin your DHT data line is plugged into
DHT tempSensor(DHT11_PIN, DHT11);

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int reg_sel = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;

float temperature = 0.0;
char handPos = 'N';
LiquidCrystal lcd(reg_sel, en, d4, d5, d6, d7);

Servo WaterMotor;
int servoPin = 36;



 /*
 * A10 - PK2 green LED (output)
 * A11 - PK3 - yellow LED (output)
 * A15 - PK7 - push button (with pulldown resistor)
*/
volatile unsigned char * port_k = (unsigned char *) 0x108;
volatile unsigned char * ddr_k = (unsigned char *) 0x107;
volatile unsigned char * pin_k = (unsigned char *) 0x106;


/*
 * Fan motor test
 * 
 * Code adapted from:
 * https://create.arduino.cc/projecthub/ingo-lohs/first-test-super-starterkit-from-elegoo-motor-3-6v-dc-5b199d
 * 
 * Entering 51 in the serial port is sufficient to get the fan moving.
 * 
 * Wiring: see Project prototpye on tinkerCAD. Transistor is a NPN PN2222; resistor is 220 ohm. Diode is "diode rectifier" from the kit. 
 * Note that white stripe on the diode goes TOWARDS the positive side of the circuit (red wire from fan).
 */

int greenLEDPin = A10;
int yellowLEDPin = A11;

int motorPin = 8; // for fan motor

// ultrasonic:
const int triggerPin = 50;
const int echoPin = 52;
long duration;
int distance;


void setup() 
{
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  tempSensor.begin(); // start collecting temperature data
  
  displayTemperature(); // displays current temperature to the top line of the LCD
  
  // set the cursor to column 0, line 1(note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  
  // For LCD:
  handPos = 'N';
  lcd.print("Hands good: ");
  lcd.print(handPos);

  // for LEDs and button
  //set pk2 and pk3 as output 
  *ddr_k |= 0x0C;
  //set PushButton as input
  *ddr_k |= 0x7F;
  // enable the pull-down resistor on PK4 (this needs to be a pull-down resistor in order to trigger the change when the button is pressed)
  *port_k |= 0x80;

  pinMode(motorPin, OUTPUT);  // fan motor
  WaterMotor.attach(servoPin); // servo motor

  // ultrasonic
  pinMode (triggerPin, OUTPUT);
  pinMode (echoPin, INPUT);


  // LEDs
  pinMode(greenLEDPin, OUTPUT);
  pinMode(yellowLEDPin, OUTPUT);

  Serial.begin(9600);

  

  // RTC - inital setup

  if (!rtcModule.isrunning()) 
  {
    Serial.println("RTC lost power, lets set the time!");
  
    // Comment out below lines once you set the date & time.
    // Following line sets the RTC to the date & time this sketch was compiled
    rtcModule.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
    // Following line sets the RTC with an explicit date & time
    // for example to set January 27 2017 at 12:56 you would call:
    // rtc.adjust(DateTime(2017, 1, 27, 12, 56, 0));
  }


  printTimeToSerial();  //RTC - print time
  
  
}

void loop()
{
  printTimeToSerial();  // print the current time to serial port- should be calibrated to local time-ish.
  while (*pin_k & 0x80) {}; // PUSH BUTTON TO BEGIN
  displayTemperature();
  distance = ultrasonic();
  // move the servo motor a bit
  
  WaterMotor.write(90);
  delay(200);

  
  // Ultrasonic and LED test - green light should come on when an object is close enough
  while (distance < 5) 
  {
    digitalWrite(yellowLEDPin, LOW);  // turn off yellow LED
    // Update LCD
    lcd.setCursor(0, 1);
    handPos = 'Y';
    lcd.print("Hands good: ");
    lcd.print(handPos);
    
    distance = ultrasonic();
  };
  digitalWrite(greenLEDPin, LOW);
  digitalWrite(yellowLEDPin, HIGH);
  
  // Update LCD
  lcd.setCursor(0, 1);
  handPos = 'N';
  lcd.print("Hands good: ");
  lcd.print(handPos);

  fanControl(90); // turn on fan
  delay(3000);   // run for 3 seconds
  fanControl(0); // turn off fan
  delay(1000);   // wait 1 second
  
  digitalWrite(yellowLEDPin, LOW);
  
  WaterMotor.write(0); // reset water motor
  delay(200);
  
  digitalWrite(greenLEDPin, HIGH);
  
}

void displayTemperature()
{ // displays current temperature to the top line of the LCDv
  lcd.setCursor(0, 0);  // set cursor to column 0, row 0 (leftmost bit of top row)
  temperature = tempSensor.readTemperature(true); // This will read temperature as Fahrenheit. To read temp as Celsius, remove the "true" argument

  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print(" F");  // or "C" if temp is displayed in Celsius
}

void fanControl(int speed)
{
  Serial.println("Speed 0 to 255, minimum 50 to start fan. 0 to stop.");

  if (speed >= 50 && speed <= 255)
  {
    analogWrite(motorPin, speed);
  }
  else if (speed == 0)
  {
    analogWrite(motorPin, speed);
  }




  /* Some ideas on how to implement this:
  if (speed >= 50 && speed <= 255)
  {
    // write int value to pin
  }
  else
  {
    // set pin to 0? 
  }

  
   * Writes an analog value (PWM wave) to a pin. Can be used to light a LED at varying brightnesses or 
   * drive a motor at various speeds. After a call to analogWrite(), the pin will generate a steady 
   * rectangular wave of the specified duty cycle until the next call to analogWrite() (or a call to 
   * digitalRead() or digitalWrite()) on the same pin.
   */
}

int ultrasonic()
{
  //ultrasonic
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);  // minimum time needed for a pulse
  digitalWrite(triggerPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034/2;  //based on speed of sound
  //Serial.print("Distance: ");
  //Serial.println(distance);

  return distance;
}

void printTimeToSerial()
{
  DateTime now = rtcModule.now();
    
  Serial.println("Current Date & Time: ");
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");

  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
}
