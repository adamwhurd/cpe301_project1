volatile unsigned char * port_d = (unsigned char *) 0x2B;
volatile unsigned char * ddr_d = (unsigned char *) 0x2A;
volatile unsigned char * pin_d = (unsigned char *) 0x29;

volatile unsigned char * port_k = (unsigned char *) 0x108;
volatile unsigned char * ddr_k = (unsigned char *) 0x107;
volatile unsigned char * pin_k = (unsigned char *) 0x106;

volatile unsigned char* myEICRA = (unsigned char*) 0x69;
volatile unsigned char* myEIMSK = (unsigned char*) 0x3D;
volatile unsigned char *mySREG = (unsigned char *) 0x5F;

#include "DHT.h"            // temp sensor
#include <LiquidCrystal.h>  // LCD
#include <Servo.h>          // servo motor
#include <Wire.h>           // Real-Time Clock (RTC)
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

volatile unsigned char * port_f = (unsigned char *) 0x31;
volatile unsigned char * ddr_f  = (unsigned char *) 0x30;
volatile unsigned char * pin_f  = (unsigned char *) 0x2F;

volatile unsigned char* myPCICR = (unsigned char*) 0x68;
volatile unsigned char* myPCMSK2 = (unsigned char*) 0x6D;

volatile DateTime currentTime;             // Object holds the current time
//volatile int timeCounter = 0;     // Duration of timer in seconds. Displayed to LCD


DHT tempSensor(DHT11_PIN, DHT11);
RTC_DS1307 rtcModule;
LiquidCrystal lcd(registerSelect, enable, data4, data5, data6, data7);
Servo WaterMotor;

 // LCD:
volatile float temperature = 0.0;
char handPos = 'N';

// ultrasonic:
volatile long duration;
volatile int distance;

volatile int count = 0;
volatile bool system_enabled;
volatile bool button_pressed;
volatile bool hands_washed;

void setup() 
{
  //button set up
  //set PushButton as input pin 18 (button) INT3 <---- this needs EIMSK 
  *ddr_d &= 0xF7;
  // enable the pull-down resistor on pin 18 (button) INT3(this needs to be a pull-down resistor in order to trigger the change when the button is pressed)
  *port_d |= 0x08;
  //set mode to 'rising' for button interrupt
  *myEICRA |= 0xC0;
  //turn on interrupt on pin 18 (button) INT3
  *myEIMSK |= 0x08;

  system_enabled = false;
  button_pressed = false;
  hands_washed = false;

  //turn on monitor
  Serial.begin(9600);
  for(int i = 0; i < 20000; ++i) {}; // simple delay to give Serial() time to get started
  //delay(2000);

  // RTC - inital setup - will only run if RTC time has not been previously set in EEPROM
  if (!rtcModule.isrunning()) 
  {
    Serial.println("Setting Real-Time");
    rtcModule.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  //turn on interrupts
  sei();

  //LEDs set up
  //set pk2, pk3 as output (LEDs) 
  *ddr_k |= 0x0C;
  //set both to low
  *port_k &= 0xF3;

  pinMode(motorPin, OUTPUT);  // fan motor
  WaterMotor.attach(servoPin); // servo motor
  tempSensor.begin(); // start collecting temperature data
  
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  pinMode (triggerPin, OUTPUT);
  pinMode (echoPin, INPUT);
  
  WaterMotor.write(0);  // ensure that servo motor is reset in case of power loss
}

void loop() 
{
  
  //lcd.clear();          // clear everything off of LCD
  lcd.setCursor(0, 0);  // set cursor to column 0, row 0 (leftmost bit of top row)
  lcd.print("Push button");
  lcd.setCursor(0, 1);  // set cursor to column 0, row 1 (leftmost bit of bottom row)
  lcd.print("to begin");

  //when system enabled
  while(system_enabled) 
  {
    //light up green led (check for now)
    //*port_k |= 0x04;
   //everytime the button is pressed the interrupt will come here (i hope) 
    if(button_pressed) 
    {
      printTimeToSerial();
      button_pressed = false;
    }
    //if hands placed in the right spot for the first time
    distance = ultrasonic();
    if(distance < 20) 
    {
      //assume hands are washed
      hands_washed = true;
    }
  
    count = 30; // 30 seconds
    WaterMotor.write(90); // turn servo motor 90 degrees
    
    while(hands_washed && (count > 0)) 
    {
      distance = ultrasonic();  // update distance
      if(distance < 30) 
      {
        /*
         * HAND-WASHING SEQUENCE
         */
        //delay(1000);
        myDelay(1);
        lcd.clear();          // clear everything off of LCD
        displayTemperature();
        displayTimer(count);
        count--;       
      }
      else 
      { 
        /*
         * USER REMOVED HANDS PREMATURELY
         */
        *port_k |= 0x08;  // turn on yellow LED
        count = 10;
        while (count > 0)
        {
          distance = ultrasonic();  // update distance
          if(distance < 30)
          { // break out and restart 30 second timer
            *port_k &= 0xF7;  // turn off yellow LED
            count = 30;       // reset timer duration to 30 seconds
            break;    
          }
          //delay(1000);
          myDelay(1);
          lcd.clear();          // clear everything off of LCD
          displayTemperature();
          displayTimer(count);
          count--;    
        } // end while (count > 0)

        // reset program - user failed to replace hands
        *port_k &= 0xF7;  // turn off yellow LED
        hands_washed = false;
        system_enabled = false;
        break;
      }
    } // end while (hands_washed)


    /*
     * FAN CYCLE
     */
    //if (hands_washed)
    //{
      WaterMotor.write(0); // reset servo motor
      *port_k |= 0x04;  // Turn on green LED
      fanControl(80);  // turn on fan
      count = 15;       // set time to 15 seconds
      while(count > 0)
      { // count down 15 seconds
        //delay(1000);
        myDelay(1);
        lcd.clear();          // clear everything off of LCD
        displayTemperature();
        displayTimer(count);
        count--;       
      }
      fanControl(0);   // turn off fan

      lcd.clear();          // clear everything off of LCD
      lcd.setCursor(0, 0);  // set cursor to column 0, row 0 (leftmost bit of top row)
      lcd.print("Good Job");

      delay(3000);     // Give fan time to spin down to avoid trying to restart it too soon
    //}
    
    /*
     * RESET
     */
    hands_washed = false;
    //system_enabled = false;
    //button_pressed = false;
    *port_k &= 0xFB;  // turn off green LED
  } // end while(system_enabled)
} // end loop


ISR(INT3_vect) 
{
    system_enabled = true;
    button_pressed = true;
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

void fanControl(int speed)
{
  //Serial.println("Speed 0 to 255, minimum 50 to start fan. 0 to stop.");

  if (speed >= 50 && speed <= 255)
  {
    analogWrite(motorPin, speed);
  }
  else if (speed == 0)
  {
    analogWrite(motorPin, speed);
  }
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

  
  int timeCounter = timeDelay;      // reset timer to 0 seconds
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

void myDelay(int delayTime)
{
  currentTime = rtcModule.now();          // Update current real time
  DateTime time1 = currentTime.second();  // holds seconds of initial timestamp
  DateTime time2 = currentTime.second();  // holds seconds of new timestamp for comparison
  TimeSpan difference = TimeSpan(time2 - time1);  // Establish inital time difference (should be 0)

  int diff = (int)difference.totalseconds();   // cast as int for easy comparison
  
  while ( (delayTime - diff) > 0)
  {
    currentTime = rtcModule.now();        // Update current real time
    time2 = currentTime.second();         // Update with new time for comparison
    difference = TimeSpan(time2 - time1); // Compare new time with initial time
    diff = (int)difference.totalseconds();   // cast as int for easy comparison
  } // end while
}
