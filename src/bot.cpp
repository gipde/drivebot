#include "MyLcd.h"
#include <Arduino.h>
//#include <LiquidCrystal.h>

#include "DigitalIO.h"
// Button used for starting the bot
#define BUTN 10

// LED's signalize the CNY70 focus
#define LED1 0
#define LED2 1
#define LED3 2

// LCD
#define LCD_RS 3
#define LCD_ENABLE 4
#define LCD_D4 5
#define LCD_D5 6
#define LCD_D6 7
#define LCD_D7 8

// SHIFT
#define SHIFT_DATA 5  // SER
#define SHIFT_LATCH 6 // RCLK
#define SHIFT_CLOCK 9 // SRCLK

#define SENSOR1 17 // SENSOR1
#define SENSOR2 18 // SENSOR2
#define SENSOR3 19 // SENSOR3

#define SHIFT_REGISTER 3

const uint8_t SOFT_SPI_MISO_PIN = 13;
const uint8_t SOFT_SPI_SS_PIN = 5;
const uint8_t SOFT_SPI_MOSI_PIN = 6;
const uint8_t SOFT_SPI_SCK_PIN = 9;
const uint8_t SPI_MODE = 0;

SoftSPI<SOFT_SPI_MISO_PIN, SOFT_SPI_MOSI_PIN, SOFT_SPI_SCK_PIN, SPI_MODE> spi;
/*
writes Data through out 2 shift Registers
*/
uint32_t data = 0;

inline void shiftWrite(int desiredPin, boolean desiredState) {
  bitWrite(data, desiredPin, desiredState);
  digitalWrite(SHIFT_LATCH, LOW);
  shiftOut(SHIFT_DATA, SHIFT_CLOCK, MSBFIRST, (byte)(data & 0xff));
  shiftOut(SHIFT_DATA, SHIFT_CLOCK, MSBFIRST, (byte)((data >> 8) & 0xff));
  shiftOut(SHIFT_DATA, SHIFT_CLOCK, MSBFIRST, (byte)(data >> 16));
  digitalWrite(SHIFT_LATCH, HIGH);
}

// Global Vars
int leds[] = {LED1, LED2, LED3};
MyLcd *lcd;

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
set initial state of Pins
*/

void setup_pins() {

  Serial.println("Settings Pins");
  pinMode(BUTN, INPUT);

  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_LATCH, OUTPUT);
  pinMode(SHIFT_CLOCK, OUTPUT);

  for (int i = 0; i < SHIFT_REGISTER * 8; i++)
    shiftWrite(i, LOW);
}

#define transfer_bit(b)                                                        \
  if (b)                                                                       \
    PORTD |= (1 << 5);                                                         \
  else                                                                         \
    PORTD &= ~(1 << 5);                                                        \
  PORTD ^= (1 << 6);                                                           \
  PORTD ^= (1 << 6);

/*
main Setup Method
*/
void setup(void) {
  Serial.begin(115200);
  Serial.println("We are starting ... ");

  DDRD |= (1 << 5) | (1 << 6);
  DDRB |= (1 << 1);

  uint32_t data = 0xfecdab;

  // duty cycle dauert 11u
  while (true) {
    digitalWrite(5, HIGH);
    digitalWrite(5, LOW);
  }

  int sh5 = (1 << 5);
  int sh6 = (1 << 6);
  int sh1 = (1 << 1);

  while (true) {
    // komplett dauert 23u
    PORTB &= ~sh1; // PD5 Low;
    transfer_bit(data & (1 << 7));
    transfer_bit(data & (1 << 6));
    transfer_bit(data & (1 << 5));
    transfer_bit(data & (1 << 4));
    transfer_bit(data & (1 << 3));
    transfer_bit(data & (1 << 2));
    transfer_bit(data & (1 << 1));
    transfer_bit(data & (1 << 0));

    transfer_bit(data & (1 << 15));
    transfer_bit(data & (1 << 14));
    transfer_bit(data & (1 << 13));
    transfer_bit(data & (1 << 12));
    transfer_bit(data & (1 << 11));
    transfer_bit(data & (1 << 10));
    transfer_bit(data & (1 << 9));
    transfer_bit(data & (1 << 8));

    transfer_bit(data & (uint32_t)(1 << 23));
    transfer_bit(data & (uint32_t)(1 << 22));
    transfer_bit(data & (uint32_t)(1 << 21));
    transfer_bit(data & (uint32_t)(1 << 20));
    transfer_bit(data & (uint32_t)(1 << 19));
    transfer_bit(data & (uint32_t)(1 << 18));
    transfer_bit(data & (uint32_t)(1 << 17));
    transfer_bit(data & (uint32_t)(1 << 16));

    PORTB |= sh1; // PD5 high;
    asm volatile("nop");
    //    delayMicroseconds(10);
  }

  while (true) {

    for (int i = 0; i < SHIFT_REGISTER * 8; i++) {
      shiftWrite(i, HIGH);
      delay(20);
    }
    for (int i = 0; i < SHIFT_REGISTER * 8; i++) {
      shiftWrite(i, LOW);
      delay(20);
    }
  }

  lcd = new MyLcd(3, 4, 5, 6, 7, 8, &shiftWrite);
  lcd->begin(20, 4);
  lcd->setCursor(0, 0);
  lcd->print("HelloWorld");
  while (true) {
  }

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
  //  lcd.setCursor(x, 1);
  //  lcd.print(value);
  return value;
}

/*
Main Loop
*/
void loop(void) {

  // shifting();

  if (iv-- == 0) {
    iv = INTV;

    clearLeds();

    lcd->clear();
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
  //  lcd.setCursor(12, 0);
  //  lcd.print(btn);

  if (!btn) {
    if (!changed) {
      motor_on = !motor_on;
      changed = true;
    }
    //  lcd.setCursor(13, 0);
    //  lcd.print(motor_on);
  } else {
    changed = false;
  }

  //  lcd.setCursor(15, 0);
  //  lcd.print(dir);
  driveMotors(dir);
}

// LiquidCrystal lcd2(10, 11, 16, 17, 18, 19);
// // set up the LCD's number of columns and rows:
// int max_x = 20;
// int max_y = 4;
// lcd2.begin(max_x, max_y);
// // Print a message to the LCD.
// for (int i = 0; i < max_x; i++) {
//   for (int j = 0; j < max_y; j++) {
//     lcd2.setCursor(i, j);
//     delay(50);
//     lcd2.print(1);
//     delay(100);
//   }
// }
// lcd2.print("hello, world!");
//
// while (true) {
//
//   lcd2.setCursor(0, 1);
//   // print the number of seconds since reset:
//   lcd2.print(millis() / 1000);
// }
// Serial.print("setting pin ");
// Serial.print(desiredPin);
// Serial.print(" to ");
// Serial.print(desiredState);
// Serial.print(" ");
// Serial.print(data);
// Serial.print(" ");
// Serial.print(data & 0xff);
// Serial.print(" ");
// Serial.print((byte)((data >> 8) & 0xff));
// Serial.print(" ");
// Serial.println((byte)(data >> 16) & 0xff);
