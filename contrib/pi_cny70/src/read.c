#include <stdio.h>
#include <wiringPi.h>

#define SPICLK 18
#define SPIMISO 23
#define SPIMOSI 24
#define SPICS 25

//TODO: Das Timing passt nicht so wie erwartet - darum das delay
void sdigitalWrite(int port,int state) {
  digitalWrite(port,state);
  delay(1);
}

int readadc(int adcnum) {
  if (adcnum > 7 || adcnum<0) {
    return -1;
  }

  sdigitalWrite(SPICS,HIGH);
  sdigitalWrite(SPICLK,LOW);
  sdigitalWrite(SPICS,LOW);

  int commandout = adcnum;
  commandout |= 0x18 ;
  commandout <<= 3 ;
  for (int i = 0;i<5;i++) {
    if (commandout & 0x80) {
      sdigitalWrite(SPIMOSI,HIGH);
    } else {
      sdigitalWrite(SPIMOSI,LOW);
    }
    commandout <<= 1;
    sdigitalWrite(SPICLK,HIGH);
    sdigitalWrite(SPICLK,LOW);
  }

  int adcout = 0;
  for (int i = 0;i<12;i++) {
    sdigitalWrite(SPICLK,HIGH);
    sdigitalWrite(SPICLK,LOW);
    adcout <<=1;
    if(digitalRead(SPIMISO))
      adcout |= 0x1;
  }
  sdigitalWrite(SPICS,HIGH);
  adcout >>=1;

  return adcout;
}

int main (void)
{
  printf ("Raspberry Pi read CNY70 \n") ;

  wiringPiSetupGpio() ;
  pinMode(SPIMOSI,OUTPUT);
  pinMode(SPIMISO,INPUT);
  pinMode(SPICLK,OUTPUT);
  pinMode(SPICS,OUTPUT);


 for (;;)
  {
    printf("Value: %d\n",readadc(1));
    delay (500) ;    // mS
  }

  return 0 ;
}
