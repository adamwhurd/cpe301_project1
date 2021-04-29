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
 * 9 - PWM pin on servo motor
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
 
#include "DHT.h"            // temp sensor
#include <LiquidCrystal.h>  // LCD
#include <Servo.h>          // servo motor
//#include <Wire.h>           // Real-Time Clock (RTC)
#include "RTClib.h"         // Real-Time Clock (RTC)
  
#define DHT11_PIN A1  // set this value to whatever board pin your DHT data line is plugged into
#define registerSelect A2 // LCD register select line
#define enable A3         // LCD enable line
#define data4 A4          // LCD data line
#define data5 A5          // LCD data line
#define data6 A6          // LCD data line
#define data7 A7          // LCD data line
#define greenLEDPin A10   // LED
#define yellowLEDPin A11  // LED
#define triggerPin A12    // ultrasonic output
#define echoPin A13       // ultrasonic input
#define motorPin 8        // fan motor 
#define servoPin 9        // servo motor


DHT tempSensor(DHT11_PIN, DHT11);
RTC_DS1307 rtcModule;
LiquidCrystal lcd(registerSelect, enable, data4, data5, data6, data7);
Servo WaterMotor;

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
//const int d4 = 4, d5 = 5, d6 = 6, d7 = 7;

 /*
 * A10 - PK2 green LED (output)
 * A11 - PK3 - yellow LED (output)
 * A15 - PK7 - push button (with pulldown resistor)
*/
volatile unsigned char * port_k = (unsigned char *) 0x108;
volatile unsigned char * ddr_k  = (unsigned char *) 0x107;
volatile unsigned char * pin_k  = (unsigned char *) 0x106;

volatile unsigned char * port_f = (unsigned char *) 0x31;
volatile unsigned char * ddr_f  = (unsigned char *) 0x30;
volatile unsigned char * pin_f  = (unsigned char *) 0x2F;

volatile unsigned char* myPCICR = (unsigned char*) 0x68;
volatile unsigned char* myPCMSK2 = (unsigned char*) 0x6D;

//bool system_enabled;
//int button = 0;

volatile DateTime currentTime;             // Object holds the current time
volatile int timeCounter = 0;     // Duration of timer in seconds. Displayed to LCD

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

 // LCD:
volatile float temperature = 0.0;
char handPos = 'N';

// ultrasonic:
volatile long duration;
volatile int distance;

void setup() 
{
  // for LEDs and button
  //set pk2 and pk3 as output 
  *ddr_k |= 0x0C;
  
  //set PushButton as input PK7(A15) <---- this needs PSMK 
  //*ddr_k |= 0x7F;
  // enable the pull-down resistor on PK7 (this needs to be a pull-down resistor in order to trigger the change when the button is pressed)
  //*port_k |= 0x80;

  //enable Pin Change Interrupt for PCINT23 (PCIE2)
  //*myPCICR |= 0x04;

  //turn on interrupt on pin PK7(A15)
  //*myPCMSK2 |= 0x80;

  //system_enabled = false;
  //sei();  // enable global interrupts

  /*
   * EXAMPLE:
   *   // Set up registers for I/O
  *portDDR_A |= 0xFF;             // set all Port A pins to output
  *portDDR_E &= 0xF7;             // set PE3 to input
  *port_E |= (0x01 << 3);         // enable pullup resistor on PE3
   * 
   */

  pinMode(motorPin, OUTPUT);  // fan motor
  WaterMotor.attach(servoPin); // servo motor

  // ultrasonic
  pinMode (triggerPin, OUTPUT);
  pinMode (echoPin, INPUT);


  // LEDs
  pinMode(greenLEDPin, OUTPUT);
  pinMode(yellowLEDPin, OUTPUT);

  Serial.begin(9600);
  for(int i = 0; i < 12000; ++i) {}; // simple delay to give Serial() time to get started
  //delay(2000);

  // RTC - inital setup - will only run if RTC time has not been previously set in EEPROM
  if (!rtcModule.isrunning()) 
  {
    Serial.println("Setting Real-Time");
    rtcModule.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
   
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  tempSensor.begin(); // start collecting temperature data
}

void loop()
{
  //currentTime = rtcModule.now();  // set current time

  /*
  //if system enabled light up green led
  while(system_enabled) 
  {
    *port_k |= 0x04;    //turn on green
    *port_k &= 0xF7;    //turn off yellow
  }

  //if system disabled light up yellow led
  while(!system_enabled) 
  {
    *port_k |= 0x08;    //turn on yellow
    *port_k &= 0xFB;    //turn off green
  }
  */
  
  /*
  //printTimeToSerial();  // print the current time to serial port- should be calibrated to local time-ish.
  displayTemperature(); // displays current temperature to the top line of the LCD
  
  // set the cursor to column 0, line 1(note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1); 
  lcd.print("Push Button");
  while (*pin_f & 0x01) {}; // PUSH BUTTON TO BEGIN
  
  displayTemperature();
  distance = ultrasonic();
  // move the servo motor a bit
  
  //WaterMotor.write(90);
  //delay(200);
  
  
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

  //fanControl(90); // turn on fan
  //delay(3000);   // run for 3 seconds
  //fanControl(0); // turn off fan
  delay(1000);   // wait 1 second
  
  digitalWrite(yellowLEDPin, LOW);
  
  //WaterMotor.write(0); // reset water motor
  delay(1000);
  
  digitalWrite(greenLEDPin, HIGH);
  */

  timer(30);  // Set a 30-second timer on the LCD
  lcd.clear();
  displayTemperature();
  lcd.setCursor(0, 1);  // set cursor to column 0, row 1 (leftmost bit of bottom row)
  lcd.print("Success"); // Handwashing successful

  //*port_k |= 0x04;    //turn on green
  //fanControl(90);     // turn on fan to speed 90
  timer(15);  // Set a 15-second timer on the LCD
  //fanControl(0);     // turn off fan
  //*port_k &= 0xFB;    //turn off green;
  lcd.clear();
  lcd.setCursor(0, 0);  // set cursor to column 0, row 1 (leftmost bit of bottom row)
  lcd.print("Have a nice day");

  //delay(5000);  // delay so that the final message can be read
  myDelay(5);
  // reset here
  
  
} // end loop()



/*
ISR(PCINT2_vect) 
{
  if(system_enabled) 
  {
    system_enabled = false;
  }
  else 
  {
    system_enabled = true;
    //currentTime = rtcModule.now();  // set current time
    //printTimeToSerial();  // print the current time to serial port- should be calibrated to local time-ish.
  }
} // End ISR
*/


void displayTemperature()
{ // displays current temperature to the top line of the LCDv
  lcd.setCursor(0, 0);  // set cursor to column 0, row 0 (leftmost bit of top row)
  temperature = tempSensor.readTemperature(true); // This will read temperature as Fahrenheit. To read temp as Celsius, remove the "true" argument

  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print(" F");  // or "C" if temp is displayed in Celsius
}

void displayTimer(int time)
{ // displays the time elapsed so far
  lcd.setCursor(0, 1);  // set cursor to column 0, row 1 (leftmost bit of bottom row)
  lcd.print("Duration: ");
  lcd.print(time);      
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
  Serial.print("Distance: ");
  Serial.println(distance);

  return distance;
}

void printTimeToSerial()
{
  currentTime = rtcModule.now();
    
  Serial.println("Current Date & Time: ");
  Serial.print(currentTime.year(), DEC);
  Serial.print('/');
  Serial.print(currentTime.month(), DEC);
  Serial.print('/');
  Serial.print(currentTime.day(), DEC);
  Serial.print(" (");

  Serial.print(currentTime.hour(), DEC);
  Serial.print(':');
  Serial.print(currentTime.minute(), DEC);
  Serial.print(':');
  Serial.print(currentTime.second(), DEC);
  Serial.println();
}

void timer(int timeDelay)
{
  currentTime = rtcModule.now();          // Update current real time
  DateTime time1 = currentTime.second();  // holds seconds of initial timestamp
  DateTime time2 = currentTime.second();  // holds seconds of new timestamp for comparison
  TimeSpan difference = TimeSpan(time2 - time1);  // Establish inital time difference (should be 0)
  int diff = (int)difference.totalseconds();   // cast as int for easy comparison

  
  timeCounter = timeDelay;      // reset timer to 0 seconds
  lcd.clear();          // clear everything off of LCD
  displayTemperature();  // Put temperature back up since it was wiped
  displayTimer(timeDelay - diff);

  while ( (timeDelay - diff) > 0)
  { // count down from timeDelay to 0
    currentTime = rtcModule.now();        // Update current real time
    time2 = currentTime.second();         // Update with new time for comparison
    difference = TimeSpan(time2 - time1); // Compare new time with initial time

    diff = (int)difference.totalseconds();   // cast as int for easy comparison
    
    if ((timeDelay - diff) < timeCounter)
    { // only update timer on LCD if one second has elapsed
      timeCounter--;
      lcd.clear();          // clear everything off of LCD
      displayTemperature();
      displayTimer(timeDelay - diff);
    }
  } // end while
}


void myDelay(int delayTime)
{
  currentTime = rtcModule.now();          // Update current real time
  DateTime time1 = currentTime.second();  // holds seconds of initial timestamp
  DateTime time2 = currentTime.second();  // holds seconds of new timestamp for comparison
  TimeSpan difference = TimeSpan(time2 - time1);  // Establish inital time difference (should be 0)

  timeCounter = 0;      // reset timer to 0 seconds
  
  while (difference.totalseconds() < delayTime)
  {
    currentTime = rtcModule.now();        // Update current real time
    time2 = currentTime.second();         // Update with new time for comparison
    difference = TimeSpan(time2 - time1); // Compare new time with initial time
  } // end while
}
