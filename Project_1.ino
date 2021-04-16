/*
 * Project 1: Handwashing station
 * Author: Adam Hurd
 * Date: 4 May 2021
 * Class: CPE 301.1106
 * Version: 1.0
 * 
 * This program will [INSERT DESCRIPTION]
 * 
 * Circuit description: 
 * [INSERT DESCRIPTION]. MAYBE LINK TO A SCHEMATIC? 
 * 
 * 
 */









void setup() 
{
  // initialize some values. This happens before there's any input, so there may not be much to put here.

}

void loop() 
{
  
  // wait here until start button is pushed

  // send current time to serial output - use "real time clock module"

  // turn on water faucet - this will be a motor. BE CAREFUL WITH MOTORS. RESEARCH HOW TO HOOK THEM UP

  // turn on water temp sensor

  // Begin displaying water temp- to LCD screen?

  // turn on hand sensor (ultrasonic)

  // begin monitoring hand position

  // (1) Start timer if hands are close enough - decrement from 30 to 0, one second at a time

  /*
   * alternate flows:
   * a) User has removed their hands before 30 seconds is finished.
   *    Results:  - BLINK warning LED for ten seconds
   *              - Reset timer on display
   *              - WAIT until user returns hands
   *              - Return to point (1)
   * b) User has not returned hands within 10 seconds of activating warning light
   *    Results:  - RESET system - wipe everything
   *              - Restart primary loop to wait for start button
   *              
   *              
   *  I wonder if we should use interrupts for this?
   */

   // If you made it this far, 30 seconds has expired and hands never left the sensor zone.

   // "Display should indicate that they have washed their hands completely"- how to show this? what does this mean?

   // Light up Green LED 

   // Turn on fan

   // set fan timer for 15 seconds- decrement to 0

   // 15 seconds has expired, kill the fan

   // system resets here. Maybe make a reset function, and then allow loop to return to the top (wait for start)
}



/*
 *  FUNCTION IDEAS:
 *  
 *    - a timer function that takes desired duration in seconds as an argument; example: timer(30) will start an internal 30 second timer
 * 
 */
