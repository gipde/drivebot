#include "MyLcd.h"
#include <Arduino.h>
//#include <LiquidCrystal.h>

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
#define SHIFT_CLOCK 5 // CLK -> PD5
#define SHIFT_DATA 6  // DATA -> PD6
#define SHIFT_LATCH 9 // ENABLE ->PB1

#define SENSOR1 17 // SENSOR1
#define SENSOR2 18 // SENSOR2
#define SENSOR3 19 // SENSOR3

#define SHIFT_REGISTER 3

const uint8_t SOFT_SPI_MISO_PIN = 13;
const uint8_t SOFT_SPI_SS_PIN = 5;
const uint8_t SOFT_SPI_MOSI_PIN = 6;
const uint8_t SOFT_SPI_SCK_PIN = 9;
const uint8_t SPI_MODE = 0;

/*
writes Data through out 2 shift Registers
*/

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
set state of Pins
*/
volatile uint32_t data = 0;
static void writeBit(uint8_t b) {
  asm volatile("ldi   r17, 8\n" // 8 bits to work
               "next_bit:\n"
               "sbrs  %0,7\n"   // skip if bit 7 is set
               "cbi   0x0b,6\n" // clear bit on port PD6
               "sbrc  %0,7\n"   // skip if bit 7 is clear
               "sbi   0x0b,6\n" // set bit on port PD6
               "rol   %0\n"     // shift rotate

               "sbi   0x0b,5\n"   // set bit on port PD5
               "cbi   0x0b,5\n"   // clear bit on port PD5
               "dec   r17\n"      // decrement by 1
               "brne  next_bit\n" // loop
               :
               : "r"(b)
               : "r17");
}

void shiftWrite() {
  asm volatile("cli\n"          // Disable Interrupts
               "cbi 0x05,1\n"   // Port B1 Low
               "cbi 0x0b,5\n"); // PORT D5 Low (Clk initial Low)
  writeBit(((char *)&data)[2]);
  writeBit(((char *)&data)[1]);
  writeBit(((char *)&data)[0]);
  asm volatile("sbi 0x05,1\n" // Port B1 High
               "sei\n");      // Enable Interrupts
}

/*
  digital Write Pin on Shift Register (45u each write)
*/
void digitalWritePin(uint8_t pin, uint8_t state) {
  state == HIGH ? data |= (1L << pin) : data &= ~(1L << pin);
  shiftWrite();
}

void setup_pins() {
  Serial.println("Settings Pins");
  pinMode(BUTN, INPUT);

  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_LATCH, OUTPUT);
  pinMode(SHIFT_CLOCK, OUTPUT);

  // set every pin  to 0
  shiftWrite();
}

/*
main Setup Method
*/

void setup(void) {
  Serial.begin(115200);
  Serial.println("We are starting ... ");

  setup_pins();

  while (true) {
    digitalWritePin(1, HIGH);
    delay(50);
    digitalWritePin(1, LOW);
    delay(50);
  }

  //  lcd = new MyLcd(3, 4, 5, 6, 7, 8, &shiftWrite);
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
    //    shiftWrite(i, HIGH);
    delay(d);
    //    shiftWrite(i, LOW);
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
    //  shiftWrite(leds[i], LOW);
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

  if (iv-- == 0) {
    iv = INTV;

    clearLeds();

    lcd->clear();
    int s1 = readPrintSensors(0, 0);
    int s2 = readPrintSensors(1, 4);
    int s3 = readPrintSensors(2, 8);

    if (s1 < s2) {
      if (s1 < s3) {
        //    shiftWrite(LED1, HIGH);
        if (dir > -3)
          dir--;
      } else {
        //  shiftWrite(LED3, HIGH);
        if (dir < 3)
          dir++;
      }
    } else {
      if (s2 < s3) {
        // shiftWrite(LED2, HIGH);
        dir = 0;
      } else {
        //        shiftWrite(LED3, HIGH);
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
