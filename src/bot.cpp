#include <Arduino.h>
#include <LiquidCrystal.h>

// Button used for starting the bot
#define BUTN 10

// LED's signalize the CNY70 focus
#define LED1 11
#define LED2 12
#define LED3 13

// SHIFT
#define SHIFT_DATA 9 //SER
#define SHIFT_LATCH 8 //RCLK
#define SHIFT_CLOCK 7 //SRCLK

int leds[]= {LED1,LED2,LED3};
LiquidCrystal lcd(0,1,2,3,4,5);

void setup_pins() {
  pinMode(BUTN,INPUT);

  for (int i=0;i<3;i++)
    pinMode(leds[i],OUTPUT);

  pinMode(SHIFT_DATA,OUTPUT);
  pinMode(SHIFT_LATCH,OUTPUT);
  pinMode(SHIFT_CLOCK,OUTPUT);

}

void setup(void) {
  lcd.begin(16, 2);
  setup_pins();
}

byte data = 0;
void shiftWrite(int desiredPin, boolean desiredState)
{
  bitWrite(data,desiredPin,desiredState);
  shiftOut(SHIFT_DATA, SHIFT_CLOCK, MSBFIRST, data);
  digitalWrite(SHIFT_LATCH, HIGH);
  digitalWrite(SHIFT_LATCH, LOW);
}

void driveMotor() {
  for (int j=0;j<200;j++)
  for (int i=0;i<8;i++) {
    shiftWrite(i,HIGH);
    delay(2);
    shiftWrite(i,LOW);
  }

}

void blinkLeds() {
  for (int i=0;i<3;i++) {
    digitalWrite(leds[i],HIGH);
    delay(1000);
    digitalWrite(leds[i],LOW);
  }
  lcd.setCursor(0,1);
  lcd.print("Blink Leds\n");

  driveMotor();

}

void loop(void) {

  if (!digitalRead(BUTN))
    blinkLeds();

  delay(100);

}
