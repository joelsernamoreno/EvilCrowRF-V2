int led = 32;

void setup() {
  pinMode(led, OUTPUT); // Declare the LED as an output
}

void loop() {
  digitalWrite(led, HIGH); // Turn the LED on
  delay(5000);
  digitalWrite(led, LOW); // Turn the LED off
  delay(5000);
}
