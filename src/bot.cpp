#include "MCP3008.h"
#include "MyLcd.h"
#include <Arduino.h>

// Button used for starting the bot
#define BUTN 10

// LED's signalize the CNY70 focus
#define LED1 17
#define LED2 18
#define LED3 19
#define LED_GREEN 20
#define LED_RED 21

// LCD
#define LCD_RS 1
#define LCD_ENABLE 2
#define LCD_D4 3
#define LCD_D5 4
#define LCD_D6 5
#define LCD_D7 6

// SHIFT
#define SHIFT_CLOCK 5 // CLK -> PD5
#define SHIFT_DATA 6  // DATA -> PD6
#define SHIFT_LATCH 9 // ENABLE ->PB1
#define SHIFT_REGISTER 3

// MOTOR
#define MOTOR1_PIN_OFFSET 8
#define MOTOR2_PIN_OFFSET 12

// ADC
// man könnte die Output Pins auch über das Schieberegister schalten
// aber dann dauert eine Abfrage länger
#define ADC_CS_PIN 11
#define ADC_CLOCK_PIN 16
#define ADC_MOSI_PIN 17
#define ADC_MISO_PIN 18

#define SENSOR1 17 // SENSOR1
#define SENSOR2 18 // SENSOR2
#define SENSOR3 19 // SENSOR3

// Global Vars
int leds[] = {LED1, LED2, LED3};
MyLcd *lcd;
MCP3008 *adc;

// Motor at default off
int motor_on = false;

// Motor Schritt-Weite - das ist die Zeit, zwischen den einzelnen Steps (z.B.
// Zeit
// bis der Magnet wirkt - mind 2000) - das regelt die Geschwindigkeit
int w_l = 3000;
int w_r = 6000;

// button - changed state
int changed = false;

/*
set state of Pins
*/
volatile uint32_t data = 0;
static void writeByte(uint8_t b) {
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
  writeByte(((char *)&data)[2]);
  writeByte(((char *)&data)[1]);
  writeByte(((char *)&data)[0]);
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

  lcd = new MyLcd(LCD_RS, LCD_ENABLE, LCD_D4, LCD_D5, LCD_D6, LCD_D7,
                  &digitalWritePin);

  adc = new MCP3008(ADC_CLOCK_PIN, ADC_MOSI_PIN, ADC_MISO_PIN, ADC_CS_PIN);

  // TODO: Calibrate Sensors
}

void clearLeds() {
  for (int i = 0; i < 3; i++) {
    //  shiftWrite(leds[i], LOW);
  }
}

int readSensor(int sensor) { return adc->readADC(sensor); }

int readPrintSensors(int sensor, int x) {
  int value = readSensor(sensor);
  //  lcd.setCursor(x, 1);
  //  lcd.print(value);
  return value;
}

// Schritt-Sequenz für motor
// Da gibts noch ein Muster, dass mehr Kraft auf den Motor ausübt
void motorStep(int offset, int thisStep) {
  switch (thisStep) {
  case 0:
    data |= (1L << (offset + 0));
    data &= ~(1L << (offset + 1));
    data &= ~(1L << (offset + 2));
    data &= ~(1L << (offset + 3));
    break;
  case 1:
    data &= ~(1L << (offset + 0));
    data |= (1L << (offset + 1));
    data &= ~(1L << (offset + 2));
    data &= ~(1L << (offset + 3));
    break;
  case 2:
    data &= ~(1L << (offset + 0));
    data &= ~(1L << (offset + 1));
    data |= (1L << (offset + 2));
    data &= ~(1L << (offset + 3));
    break;
  case 3:
    data &= ~(1L << (offset + 0));
    data &= ~(1L << (offset + 1));
    data &= ~(1L << (offset + 2));
    data |= (1L << (offset + 3));
    break;
  }
  shiftWrite();
}

/*
Steuert den Motor
*/
void motorLoop(void (*f)()) {
  uint32_t next_l = micros() + w_l;
  uint32_t next_r = micros() + w_r;

  int step_l = 0;
  int step_r = 0;
  while (true) {
    if (micros() > next_l) {
      if (motor_on)
        motorStep(MOTOR1_PIN_OFFSET, step_l++);
      if (step_l > 3)
        step_l = 0;
      next_l += w_l;
    }
    if (micros() > next_r) {
      if (motor_on)
        motorStep(MOTOR2_PIN_OFFSET, step_r--);
      if (step_r < 0)
        step_r = 3;
      next_r += w_r;
    }
    f();
  }
}

// diese Funktion wird aufgerufen, sobald der Motor alle aktuell anfallenden
// Schritte gemacht hat.
void sensorLoop() {

  // if (iv-- == 0) {
  //   iv = INTV;
  //
  //   clearLeds();
  //
  //   lcd->clear();
  //   int s1 = readPrintSensors(0, 0);
  //   int s2 = readPrintSensors(1, 4);
  //   int s3 = readPrintSensors(2, 8);
  //
  //   if (s1 < s2) {
  //     if (s1 < s3) {
  //       //    shiftWrite(LED1, HIGH);
  //       if (dir > -3)
  //         dir--;
  //     } else {
  //       //  shiftWrite(LED3, HIGH);
  //       if (dir < 3)
  //         dir++;
  //     }
  //   } else {
  //     if (s2 < s3) {
  //       // shiftWrite(LED2, HIGH);
  //       dir = 0;
  //     } else {
  //       //        shiftWrite(LED3, HIGH);
  //       if (dir < 3)
  //         dir++;
  //     }
  //   }
  //   if (!motor_on)
  //     delay(50);
  // }
  //
  int btn = digitalRead(BUTN);
  lcd->setCursor(0, 0);
  lcd->print(btn);

  if (!btn) {
    if (!changed) {
      motor_on = !motor_on;
      changed = true;
    }
    lcd->setCursor(1, 0);
    lcd->print(motor_on);
  } else {
    changed = false;
  }
}

/*
Main Loop
*/
void loop(void) {

  lcd->begin(20, 4);

  // motorLoop(&sensorLoop);

  while (true) {
    lcd->setCursor(0, 0);
    lcd->print(readSensor(0));
    delay(100);
  }
}
