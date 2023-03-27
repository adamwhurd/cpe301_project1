
To Run:
Assemble the circuit (see project_layout.PNG) and upload this file to the Arduino.

This program was written for the Arduino/AVR Mega 2560.



Handwashing Station
CPE 301.1001 Final Project
Authors: Adam Hurd and Julia Adamczyk
Date: 4 May 2021
Version: 1.0

This program simulates a semi-automated handwashing station. There are several major hardware components:
   - The LCD screen displays messages, such as time remaining and the current water temperature.
   - The servo motor simulates a valve on a water faucet
   - The fan motor simulates a blower for drying hands
   - The ultrasonic sensor detects hand proximity

PROGRAM FLOW:
The system is started with the pushbutton. Once pressed, after a brief delay the ultrasonic will detect whether 
the user's hands are close enough. If not, reset system and start over. If they are, the program will move to
the normal hand-washing sequences: for 30 seconds, the water's temperature will be displayed along with the
remaining time. If 30 seconds expires, a green light will activate and a fan will turn on to dry the user's hands.
If the user removes their hands before 30 seconds is up, the program will go to a warning sequence. The user will
Then have 10 seconds to reposition their hands properly. If they do, then the countdown is reset to 30 seconds and
normal handwashing resumes. if they do not, then the system terminates the sequence and resets.

In any event, the system is always reset at the bottom of loop(). 



For a visual exmaple of the circuit and required parts, see:
https://www.tinkercad.com/things/iI0ye0VSLKj (requires account; click "Simulate" to see labels)

OR

project_layout.PNG
