void setup() {
  // put your setup code here, to run once:
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  Serial.begin(38400);

}

void loop() {
  int val;
  val = analogRead(A0);
  Serial.print(val);
  Serial.print('\n');
  delay(10);              // wait for a second
}

