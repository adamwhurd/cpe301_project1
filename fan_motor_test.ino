/*
 * Fan motor test
 * 
 * Code adapted from:
 * https://create.arduino.cc/projecthub/ingo-lohs/first-test-super-starterkit-from-elegoo-motor-3-6v-dc-5b199d
 * 
 * Entering 51 in the serial port is sufficient to get the fan moving.
 * 
 * Wiring: see Project prototpye on tinkerCAD. Transistor is a NPN PN2222; resistor is 200 ohm. Diode is "diode rectifier" from the kit. 
 * Note that white stripe on the diode goes TOWARDS the positive side of the circuit (red wire from fan).
 */



int motorPin = 8;
 
void setup() 
{ 
  pinMode(motorPin, OUTPUT);
  Serial.begin(9600);
  while (! Serial);
  Serial.println("Speed 0 to 255");
  Serial.println("But the advice 50 to 255. Because the minimum voltage required to start the motor is 50.");
} 
 
 
void loop() 
{ 
  if (Serial.available())
  {
    int speed = Serial.parseInt();
    if (speed >= 50 && speed <= 255)
    {
      analogWrite(motorPin, speed);
    }
  }
} 
