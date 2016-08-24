#include <Arduino.h>
#include <LiquidCrystal.h>

// Button used for starting the bot
#define BUTN 10

// LED's signalize the CNY70 focus
#define LED1 11
#define LED2 12
#define LED3 13

// SHIFT
#define SHIFT_DATA 9  // SER
#define SHIFT_LATCH 8 // RCLK
#define SHIFT_CLOCK 7 // SRCLK

// Global Vars
int leds[] = {LED1, LED2, LED3};
LiquidCrystal lcd(6, 5, 4, 3, 2, 1);

// Motor at default off
int motor_on = false;

// Intervall of sensor-reads / motor-cycles
#define INTV 10
int iv = INTV;

// motor direction
int dir = 0;

// button - changed state
int changed = false;

byte data = 0;
void shiftWrite(int desiredPin, boolean desiredState) {
  bitWrite(data, desiredPin, desiredState);
  shiftOut(SHIFT_DATA, SHIFT_CLOCK, MSBFIRST, data);
  digitalWrite(SHIFT_LATCH, HIGH);
  digitalWrite(SHIFT_LATCH, LOW);
}

void setup_pins() {
  pinMode(BUTN, INPUT);

  for (int i = 0; i < 3; i++)
    pinMode(leds[i], OUTPUT);

  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_LATCH, OUTPUT);
  pinMode(SHIFT_CLOCK, OUTPUT);

  for (int i = 0; i < 8; i++)
    shiftWrite(i, LOW);
}
void setup(void) { setup_pins(); }

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
    digitalWrite(leds[i], LOW);
  }
}

int readSensor(int sensor) { return analogRead(sensor); }

int readPrintSensors(int sensor, int x) {
  int value = readSensor(sensor);
  lcd.setCursor(x, 1);
  lcd.print(value);
  return value;
}

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
        digitalWrite(LED1, HIGH);
        if (dir > -3)
          dir--;
      } else {
        digitalWrite(LED3, HIGH);
        if (dir < 3)
          dir++;
      }
    } else {
      if (s2 < s3) {
        digitalWrite(LED2, HIGH);
        dir = 0;
      } else {
        digitalWrite(LED3, HIGH);
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
