#include "ShiftLiquidCrystal.h"
#include <Arduino.h>

// Button used for starting the bot
#define BUTN 10

// LED's signalize the CNY70 focus
#define LED1 0
#define LED2 1
#define LED3 2

// SHIFT
#define SHIFT_DATA 5  // SER
#define SHIFT_LATCH 6 // RCLK
#define SHIFT_CLOCK 9 // SRCLK

#define SENSOR1 17 // SENSOR1
#define SENSOR2 18 // SENSOR2
#define SENSOR3 19 // SENSOR3

// Global Vars
int leds[] = {LED1, LED2, LED3};
LiquidCrystal lcd(SHIFT_LATCH);

// Motor at default off
int motor_on = false;

// Intervall of sensor-reads / motor-cycles
#define INTV 10
int iv = INTV;

// motor direction
int dir = 0;

// button - changed state
int changed = false;

/*
writes Data through out 2 shift Registers
*/
uint16_t data = 0;
void shiftWrite(int desiredPin, boolean desiredState) {
  bitWrite(data, desiredPin, desiredState);
  digitalWrite(SHIFT_LATCH, LOW);
  shiftOut(SHIFT_DATA, SHIFT_CLOCK, MSBFIRST, data & 0xff);
  shiftOut(SHIFT_DATA, SHIFT_CLOCK, MSBFIRST, (byte)(data >> 8));
  digitalWrite(SHIFT_LATCH, HIGH);
}

/*
set initial state of Pins
*/
void setup_pins() {
  pinMode(BUTN, INPUT);

  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_LATCH, OUTPUT);
  pinMode(SHIFT_CLOCK, OUTPUT);

  for (int i = 0; i < 16; i++)
    shiftWrite(i, LOW);
}

/*
main Setup Method
*/
void setup(void) {
  Serial.begin(115200);
  Serial.print("We are starting ... ");
  setup_pins();
  // TODO: Calibrate Sensors
}

/*
drives a motor
*/
void driveMotor(int motor, int d) {

  // if no delay -> motor stop
  if (!d)
    return;

  // select motor and do 1 wave clycle
  int spin = motor * 4;
  for (int i = spin; i < 4 + spin; i++) {
    shiftWrite(i, HIGH);
    delay(d);
    shiftWrite(i, LOW);
  }
}

void driveMotors(int val) {

  // adjust value to array
  val += 3;

  // speed table
  int w1[] = {0, 10, 5, 2, 2, 2, 2};
  int w2[] = {2, 2, 2, 2, 5, 10, 0};

  if (motor_on) {
    driveMotor(0, w1[val]);
    driveMotor(1, w2[val]);
  }
}

void clearLeds() {
  for (int i = 0; i < 3; i++) {
    shiftWrite(leds[i], LOW);
  }
}

int readSensor(int sensor) { return analogRead(sensor); }

int readPrintSensors(int sensor, int x) {
  int value = readSensor(sensor);
  lcd.setCursor(x, 1);
  lcd.print(value);
  return value;
}

/*
Main Loop
*/
void loop(void) {

  if (iv-- == 0) {
    iv = INTV;

    clearLeds();

    lcd.clear();
    int s1 = readPrintSensors(0, 0);
    int s2 = readPrintSensors(1, 4);
    int s3 = readPrintSensors(2, 8);

    if (s1 < s2) {
      if (s1 < s3) {
        shiftWrite(LED1, HIGH);
        if (dir > -3)
          dir--;
      } else {
        shiftWrite(LED3, HIGH);
        if (dir < 3)
          dir++;
      }
    } else {
      if (s2 < s3) {
        shiftWrite(LED2, HIGH);
        dir = 0;
      } else {
        shiftWrite(LED3, HIGH);
        if (dir < 3)
          dir++;
      }
    }
    if (!motor_on)
      delay(50);
  }

  int btn = digitalRead(BUTN);
  lcd.setCursor(12, 0);
  lcd.print(btn);

  if (!btn) {
    if (!changed) {
      motor_on = !motor_on;
      changed = true;
    }
    lcd.setCursor(13, 0);
    lcd.print(motor_on);
  } else {
    changed = false;
  }

  lcd.setCursor(15, 0);
  lcd.print(dir);
  driveMotors(dir);
}
