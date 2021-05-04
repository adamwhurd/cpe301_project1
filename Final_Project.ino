/*
 * Handwashing Station
 * CPE 301.1001 Final Project
 * Authors: Adam Hurd and Julia Adamczyk
 * Date: 4 May 2021
 * Version: 1.0
 * 
 * This program simulates a semi-automated handwashing station. There are several major hardware components:
 *    - The LCD screen displays messages, such as time remaining and the current water temperature.
 *    - The servo motor simulates a valve on a water faucet
 *    - The fan motor simulates a blower for drying hands
 *    - The ultrasonic sensor detects hand proximity
 * 
 * PROGRAM FLOW:
 * The system is started with the pushbutton. Once pressed, after a brief delay the ultrasonic will detect whether 
 * the user's hands are close enough. If not, reset system and start over. If they are, the program will move to
 * the normal hand-washing sequences: for 30 seconds, the water's temperature will be displayed along with the
 * remaining time. If 30 seconds expires, a green light will activate and a fan will turn on to dry the user's hands.
 * If the user removes their hands before 30 seconds is up, the program will go to a warning sequence. The user will
 * then have 10 seconds to reposition their hands properly. If they do, then the countdown is reset to 30 seconds and
 * normal handwashing resumes. if they do not, then the system terminates the sequence and resets.
 * 
 * In any event, the system is always reset at the bottom of loop(). 
 * 
 * For a visual exmaple of the circuit, see:
 * https://www.tinkercad.com/things/iI0ye0VSLKj
 */



/* REGISTER ADDRESSES */
// GPIO
volatile unsigned char * port_d = (unsigned char *) 0x2B;   // Board pins 18-21
volatile unsigned char * ddr_d  = (unsigned char *) 0x2A;
volatile unsigned char * pin_d  = (unsigned char *) 0x29;

volatile unsigned char * port_k = (unsigned char *) 0x108;  // Board pins A8-A15
volatile unsigned char * ddr_k  = (unsigned char *) 0x107;
volatile unsigned char * pin_k  = (unsigned char *) 0x106;

volatile unsigned char * port_h = (unsigned char *) 0x102;  // Board Pins 6-9
volatile unsigned char * ddr_h  = (unsigned char *) 0x101;
volatile unsigned char * pin_h  = (unsigned char *) 0x100;

volatile unsigned char * port_f = (unsigned char *) 0x31;   // Board pins A0-A7
volatile unsigned char * ddr_f  = (unsigned char *) 0x30;
volatile unsigned char * pin_f  = (unsigned char *) 0x2F;

// Interrupts
volatile unsigned char * myEICRA = (unsigned char*) 0x69;   // External Interrupt Control Register A
volatile unsigned char * myEIMSK = (unsigned char*) 0x3D;   // External Interrupt Mask Register
volatile unsigned char * mySREG  = (unsigned char *) 0x5F;  // AVR Status Register

// Timer variables
volatile unsigned char *myTCCR1A = (unsigned char *) 0x80; // Timer/Counter 1 Control Reg. A
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81; // Timer/Counter 1 Control Reg. B
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82; // Timer/Counter 1 Control Reg. C
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F; // Timer/Counter 1 Interrupt Mask Register
volatile unsigned int  *myTCNT1  = (unsigned  int *) 0x84; // Timer/Counter 1 (lower byte) - NOTE: do not modify while running
volatile unsigned char *myTIFR1  = (unsigned char *) 0x36; // Timer/Counter 1 Interrupt Flag Register
volatile unsigned char *portB    = (unsigned char *) 0x25; // Port B Data Register
volatile unsigned char *portDDRB = (unsigned char *) 0x24; // Port B Data Direction Register (sets I/O state)
volatile unsigned int  ticks     = 0;                      // used to calculate delay timing for frequency generation

/* LIBRARIES */
#include "DHT.h"            // temp sensor
#include <LiquidCrystal.h>  // LCD
#include <Servo.h>          // servo motor
#include "RTClib.h"         // Real-Time Clock (RTC)
//#include <Wire.h>         // Real-Time Clock (RTC)

/* MASTER PIN-OUT */
#define DHT11_PIN A1      // Temperature sensor input (pf1)
#define registerSelect A2 // LCD register select line (pf2)
#define enable A3         // LCD enable line    (pf3)
#define data4 A4          // LCD data line      (pf4)
#define data5 A5          // LCD data line      (pf5)
#define data6 A6          // LCD data line      (pf6)
#define data7 A7          // LCD data line      (pf7)
#define greenLEDPin A10   // LED                (pk2)
#define yellowLEDPin A11  // LED                (pk3)
#define triggerPin A12    // ultrasonic output  (pk4)
#define echoPin A13       // ultrasonic input   (pk5)
#define motorPin 8        // fan motor          (ph5)
#define servoPin 9        // servo motor        (ph6)
// RTC uses SDA and SCL pins

/* HARDWARE OBJECTS */
DHT tempSensor(DHT11_PIN, DHT11); // Temperature sensor
RTC_DS1307 rtcModule;             // Real-Time Clock module
LiquidCrystal lcd(registerSelect, enable, data4, data5, data6, data7);  // LCD Screen
Servo WaterMotor;                 // Water faucet motor

/* VARIABLES */
// LCD display:
volatile float temperature = 0.0;

// ultrasonic:
volatile long duration;   // Length of time bewteen the outbound pulse and the reflected echo
volatile int distance;    // distance of an object from the ultrasonic sensor in centimeters

// Misc.
volatile int  countdown = 0;  // used for several countdown sequences
volatile bool system_enabled; // set true inside the interrupt service routine (ISR) when the button is pressed
volatile bool button_pressed; // set true inside the ISR when the button is pressed
volatile bool hands_position_good;   // set true if the user positions their hands properly at the start of the hand washing sequence

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
  hands_position_good = false;

  //turn on monitor
  Serial.begin(9600);
  //for(int i = 0; i < 20000; ++i) {}; // simple delay to give Serial() time to get started

  /* RTC MODULE */
  // Turn on RTC
  if (! rtcModule.begin()) 
  { 
    Serial.println("RTC not found");
  }
  // RTC - inital setup - will only run if RTC time has not been previously set in EEPROM
  if (!rtcModule.isrunning()) 
  {
    Serial.println("Setting Real-Time");
    rtcModule.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  //turn on interrupts
  sei();    //or *mySREG |= 0x80;

  //LEDs set up
  //set pk2, pk3 as output (LEDs) 
  *ddr_k |= 0x0C;
  //set both to low
  *port_k &= 0xF3;

  //set fan motor pin 8(ph5) to output
  *ddr_h |= 0x20;              // fan motor

  WaterMotor.attach(servoPin); // servo motor
  tempSensor.begin(); // start collecting temperature data
  
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  //set triggerPin to output
  *ddr_k |= 0x10;
  //set echoPin to input
  *ddr_k &= 0xDF;

   // Initialize timer control registers
  *myTCCR1A = 0x00;
  *myTCCR1B = 0x00;
  *myTCCR1C = 0x00;
  
  ticks = calc_ticks(1);  // Set 1 second timer-value
  
  WaterMotor.write(0);  // ensure that servo motor is reset in case of power loss
}

void loop() 
{
  // Display prompt to LCD
  lcd.clear();          // clear everything off of LCD
  lcd.setCursor(0, 0);  // set cursor to column 0, row 0 (leftmost bit of top row)
  lcd.print("Push button");
  lcd.setCursor(0, 1);  // set cursor to column 0, row 1 (leftmost bit of bottom row)
  lcd.print("to begin");

  //when system enabled
  while(system_enabled) 
  {
    /*
     * MAIN LOOP
     */
    // everytime the button is pressed the interrupt will come here 
    if(button_pressed) 
    {
      printTimeToSerial();  // print the real-time to the serial port
      button_pressed = false;
    }
    /*
     * CHECK INITAL HAND POSITION
     */
    countdown = 3;
    while(countdown >= 0)
    { // count down 3 seconds
      delaySecond();
      lcd.clear();          
      lcd.setCursor(0, 0);  // set cursor to column 0, row 0 (leftmost bit of top row)
      lcd.print("POSITION HANDS");
      displayTimer(countdown);
      countdown--;       
    }
    
    //if hands placed in the right spot for the first time, continue with hand-washing sequence
    distance = ultrasonic();
    if(distance < 15) 
    {
      // Hands are properly positioned
      hands_position_good = true;
    }
  
    countdown = 30; // 30 second countdown
    WaterMotor.write(90); // turn servo motor 90 degrees
    
    while(hands_position_good && (count >= 0)) 
    {
      distance = ultrasonic();  // update distance
      if(distance < 15) 
      { // Countdown from 30
        /*
         * HAND-WASHING SEQUENCE
         */
        delaySecond();
        lcd.clear();       
        displayTemperature();
        displayTimer(countdown);
        countdown--;       
      }
      else 
      { 
        /*
         * USER REMOVED HANDS PREMATURELY
         */
        *port_k |= 0x08;  // turn on yellow LED
        countdown = 10;
        while (countdown >= 0)
        { // Count down from 10
          distance = ultrasonic();  // update distance
          if(distance < 30)
          { // break out and restart 30 second timer
            /*
             * USER RETURNED HANDS
             */
            *port_k &= 0xF7;  // turn off yellow LED
            countdown = 30;       // reset timer duration to 30 seconds
            break;            // Return to hand-washing sequence
          }
          delaySecond();
          lcd.clear();          // clear everything off of LCD
          lcd.setCursor(0, 0);  // set cursor to column 0, row 0 (leftmost bit of top row)
          lcd.print("Return hands");
          displayTimer(countdown);
          countdown--;    
        } // end while (count > 0)

        if (countdown <= 0)
        {
          // reset program - user failed to replace hands
          *port_k &= 0xF7;  // turn off yellow LED
          hands_position_good = false;
          system_enabled = false;
          break;
        } // end if (countdown <= 0)
      } // end if-else (distance < 15) 
    } // end while (hands_position_good)

    /*
     * FAN CYCLE
     */
    if (hands_position_good)
    {
      WaterMotor.write(0); // reset servo motor
      *port_k |= 0x04;  // Turn on green LED
      fanControl(80);   // turn on fan
      countdown = 15;       // set time to 15 seconds
      while(countdown >= 0)
      { // count down 15 seconds
        delaySecond();
        lcd.clear();          // clear everything off of LCD
        displayTemperature();
        displayTimer(countdown);
        countdown--;
      }
      *port_k &= 0xFB;  // Turn off green LED
      fanControl(0);   // turn off fan
      lcd.clear();          // clear everything off of LCD
      lcd.setCursor(0, 0);  // set cursor to column 0, row 0 (leftmost bit of top row)
      lcd.print("Good Job");
      delaySecond();
      delaySecond();   // Give fan time to spin down to avoid trying to restart it too soon
    } // end if (hands_position_good)

    /*
     * RESET
     */
    countdown = 3;
    while(countdown >= 0)
    { // count down 3 seconds
      delaySecond();
      lcd.clear();          
      lcd.setCursor(0, 0);  // set cursor to column 0, row 0 (leftmost bit of top row)
      lcd.print("System");
      lcd.setCursor(0, 1);  // set cursor to column 0, row 1 (leftmost bit of bottom row)
      lcd.print("Resetting");
      countdown--;       
    } // end while(countdown >= 0)

    WaterMotor.write(0); // reset servo motor
    *port_k &= 0xF7;  // turn off yellow LED if it's on
    *port_k &= 0xFB;  // turn off green LED if it's on
    hands_position_good = false;
    system_enabled = false;
  } // end while(system_enabled)
} // end loop


ISR(INT3_vect) 
{
    system_enabled = true;
    button_pressed = true;
}

int ultrasonic()
{ // Return the distance of an object from the sensor in centimeters
  //set triggerPin to output
  *ddr_k |= 0x10;
  //set echoPin to input
  *ddr_k &= 0xDF;

  // set triggerPin low
  *port_k &= 0xEF;

  ticks = calc_ticks(500000); // 500k Hz = 2 microseconds
  delayMicroSecond();

  // set triggerPin high
  *port_k |= 0x10;

  ticks = calc_ticks(100000); // 100k Hz = 10 microseconds
  delayMicroSecond();         // 10 microseconds is the minimum time needed for a pulse

  // set triggerPin low
  *port_k &= 0xEF;

  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034/2;  // based on speed of sound at sealevel; returns distance in centimeters

  return distance;
}

void fanControl(int speed)
{
  // The speed must be between 0 to 255. It takes a minimum of 50 to start fan. 0 to stop.
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
  DateTime currentTime = rtcModule.now();
  Serial.println("Current Date & Time: ");

  // Print date
  Serial.print(currentTime.year(), DEC);
  Serial.print('/');
  Serial.print(currentTime.month(), DEC);
  Serial.print('/');
  Serial.print(currentTime.day(), DEC);
  Serial.print(" (");

  // Print time
  Serial.print(currentTime.hour(), DEC);
  Serial.print(':');
  Serial.print(currentTime.minute(), DEC);
  Serial.print(':');
  Serial.print(currentTime.second(), DEC);
  Serial.print(")");
  Serial.println();
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

void delaySecond() 
{ // Make the system wait for some amount of seconds
    // stop the timer
  *myTCCR1B &= 0xF8;
  
  // set the counts
  *myTCNT1 = (unsigned int) (65536 - ticks);
  
  // start the timer
  *myTCCR1B |= 0x04; // 256 pre-scaler
  
  // wait for overflow
  while((*myTIFR1 & 0x01)==0);
  
  // stop the timer
  *myTCCR1B &= 0xF8; 
  
  // reset TOV
  *myTIFR1 |= 0x01;
}

unsigned int calc_ticks(unsigned int freq) 
{ // Calculate how many ticks are needed for a sound based on the frequency
  volatile unsigned int prescale = 256;             // Necessary to slow system clock sufficiently
  volatile double period = (double) 1.0/freq;     // Period = 1 / F    
  volatile double clock_freq = (double)(16000000.0/prescale); // system is 16 MHz
  
  return (unsigned int) clock_freq; // ticks per cycle
}

void delayMicroSecond() 
{ // Make the system wait for some amount of seconds
    // stop the timer
  *myTCCR1B &= 0xF8;
  
  // set the counts
  *myTCNT1 = (unsigned int) (65536 - ticks);
  
  // start the timer
  * myTCCR1B |= 0x01; // no prescale
  
  // wait for overflow
  while((*myTIFR1 & 0x01)==0);
  
  // stop the timer
  *myTCCR1B &= 0xF8; 
  
  // reset TOV
  *myTIFR1 |= 0x01;
}
