/*
    Code for 2017 Season Box Bots robot control board
    Arduino Nano clone w/ CH340 USB connected to TB6612 motor driver board

    CH1 - Steering
    CH2 - Throttle
    CH3 - Other
*/

#define   lpwm    9     // pulse width modulation for left motor is pin 3
#define   lpin1   7    // left control pin one is pin 4
#define   lpin2   8     // left control pin two is pin 5
#define   standby 6     // standby pin is 6 - LOW=motor driver off, HIGH=motor driver on for new board
#define   rpin1   5     // right control pin one is pin 7
#define   rpin2   4     // right control pin two is pin 8
#define   rpwm    3     // pulse width modulation for right motor is pin 9 for new board 

#define   forward 0
#define   reverse 1
#define   coast   2
#define   brake   3
#define   rightMotor  0
#define   leftMotor 1

#define   in_ch1  A0    // input channel one is on Steering
#define   in_ch2  A1    // input channel two is on Throttle
#define   in_ch3  A2    // input channel three is on Other
#define   maxWait 25000 // longest time in uS to wait for a radio pulse

// states for the flipper servo!
#define   steadyOpening 0 // not moving now, but will open when next delta of button
#define   opening       1 // steadily opening up, but will steadyClosing on next delta of button 
#define   steadyClosing 2 // not moving now, but will close when next delta of button
#define   closing       3 // steadily chomping down, will be steadyOpening on next delta of button

#define   allDown       1079 // uS pulse width that commands flipper all the way down
#define   allUp         2380 // pulse width that commands flipper all the way up

#define   flipperPin    11 // where the flipper servo is connected

int currentPosition = allDown;
bool priorBtnState = false;
bool currentBtnState = false;
int currentState = 0;


int ch1; // Steering - Joystick x-axis
int ch2; // Thottle - Joystick y-axis
int ch3; // Weapon Switch
int throttle = 0;
int spin = 0;
int rightMotorSpeed = 0;
int leftMotorSpeed = 0;
byte  oldDirection = 0; //for troubleshooting stuttering left motor problem
byte  newDirection = 0;


void motorFunction(byte function, byte motor) {
  switch (motor) {
    case leftMotor:
      switch (function) {
        case forward:
          digitalWrite(lpin1, HIGH);
          digitalWrite(lpin2, LOW);
          break;
        case reverse:
          digitalWrite(lpin1, LOW);
          digitalWrite(lpin2, HIGH);
          break;

        case brake:
          digitalWrite(lpin1, HIGH);
          digitalWrite(lpin2, HIGH);
          break;

        default:  // coast condition
          digitalWrite(lpin1, LOW);
          digitalWrite(lpin2, LOW);
          break;
      }
      break;
    case rightMotor:
      switch (function) {
        case forward:
          digitalWrite(rpin1, HIGH);
          digitalWrite(rpin2, LOW);
          break;

        case reverse:
          digitalWrite(rpin1, LOW);
          digitalWrite(rpin2, HIGH);
          break;

        case brake:
          digitalWrite(rpin1, HIGH);
          digitalWrite(rpin2, HIGH);
          break;

        default:  // coast condition
          digitalWrite(rpin1, LOW);
          digitalWrite(rpin2, LOW);
      }
      break;
    default:
      break;
  }
}

void setup() {
  // put your setup code here, to run once:
  pinMode(lpwm, OUTPUT);
  pinMode(lpin1, OUTPUT);
  pinMode(lpin2, OUTPUT);
  pinMode(rpwm, OUTPUT);
  pinMode(rpin1, OUTPUT);
  pinMode(rpin2, OUTPUT);
  pinMode(standby, OUTPUT);

  pinMode(in_ch1, INPUT);       // channel one of RC receiver, x-axis steering
  pinMode(in_ch2, INPUT);       // channel two of RC receiver, y-axis throttle
  pinMode(in_ch3, INPUT);       // channel three of RC receiver, "other"
  
  pinMode(flipperPin, OUTPUT);  // servo driving pin

  digitalWrite(lpin1, LOW);
  digitalWrite(lpin2, LOW);
  digitalWrite(rpin1, LOW);
  digitalWrite(rpin2, LOW);
  digitalWrite(standby, HIGH);
  digitalWrite(flipperPin, LOW);  // turn on the things

  currentBtnState = pulseIn(in_ch3, HIGH, maxWait) > 1750;
  priorBtnState = currentBtnState;

/**
 * Attaching lifter to pin 10 or 11 breaks the left motor!
 * Might have to do this with direct writing to the pin with uS delay :\
 */
//  lifter.write(allDown);
  Serial.begin(9600);
}

void loop() { // about 31 Hz is the loop speed TODO, maybe different order of pulseIn would give better speeds?
  // pulsein returning value of 1000 to 2000 (1500 default neutral position)
  // All Numbers are with transmitter channels in Normal position
  ch1 = pulseIn(in_ch1, HIGH, maxWait); // Steering : 1000 Left, 2000 Right
  ch2 = pulseIn(in_ch2, HIGH, maxWait); // Throttle : 1000 Reverse, 2000 Forward
  currentBtnState = pulseIn(in_ch3, HIGH, maxWait) > 1750;

  if (currentBtnState != priorBtnState) { // sequential state engine
    currentState++;
  }
  if (currentState > closing) { // if greater than last state code, go to initial mode.
    currentState = steadyOpening;
  }
  priorBtnState = currentBtnState;

  switch (currentState) {
    case steadyOpening:
//      Serial.print("steadyOpening ");
//      Serial.println(currentPosition);
      break;
    case opening:
      currentPosition += 100;
//      Serial.print("opening ");
//      Serial.println(currentPosition);
      break;
    case steadyClosing:
//      Serial.print("steadyClosing ");
//      Serial.println(currentPosition);
      break;
    case closing:
      currentPosition -= 100;
//      Serial.print("closing");
//      Serial.println(currentPosition);
      break;
    default:
//      Serial.print("Unknown State: ");
//      Serial.println(currentState);
      break;
  }

  if (currentPosition < allDown) {
    currentState++;
    currentPosition = allDown;
  } else if (currentPosition > allUp) {
    currentState++;
    currentPosition = allUp;
  }

  // handle the case in which the signals time
  if (ch1 < 800) {
    ch1 = 1500;
  }
  if (ch2 < 800) {
    ch2 = 1500;
  }

  ch1 = map(ch1, 1000, 2000, -255, 255); //center over 500
  ch2 = map(ch2, 1000, 2000, -255, 255); //center over 500

  if (abs(ch1) < 10) {
    ch1 = 0;
  }
  if (abs(ch2) < 6) {
    ch2 = 0;
  }
  spin = -1 * ch1;
  throttle = -1 * ch2;

  rightMotorSpeed = constrain( throttle + spin, -255, 255);
  leftMotorSpeed = constrain( throttle - spin, -255, 255 );

  if (rightMotorSpeed < 0) {  // outside deadband, in reverse
    //   Serial.print(" Right Back ");
    motorFunction(reverse, rightMotor);
  }
  else {
    //   Serial.print(" Right Fwd ");
    motorFunction(forward, rightMotor);
  }
  if (leftMotorSpeed < 0) {
    //   Serial.print(" Left Back ");
    motorFunction(reverse, leftMotor);
    newDirection = reverse;
  }
  else {
    newDirection = forward;
    //   Serial.print(" Left Fwd ");
    motorFunction(forward, leftMotor);
    leftMotorSpeed = (int)(leftMotorSpeed * 0.85);
  }
  if (oldDirection != newDirection) {
    //    Serial.print("@");
  }
  oldDirection = newDirection;

  analogWrite(lpwm, abs(leftMotorSpeed));
  analogWrite(rpwm, abs(rightMotorSpeed));


  digitalWrite(flipperPin, HIGH);
  delayMicroseconds(currentPosition);
  digitalWrite(flipperPin, LOW);

}
