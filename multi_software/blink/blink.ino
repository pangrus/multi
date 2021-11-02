/*
                   _ _   _
                  | | | (_)
   _ __ ___  _   _| | |_ _
  | '_ ` _ \| | | | | __| |
  | | | | | | |_| | | |_| |
  |_| |_| |_|\__,_|_|\__|_|

    blink
    -----------
    blinks all leds

    pangrus 2021
*/

int blinkTime = 200;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_LED3, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, LOW);
  delay(blinkTime);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(blinkTime);
  digitalWrite(PIN_LED3, LOW);
  delay(blinkTime);
  digitalWrite(PIN_LED3, HIGH);
  delay(blinkTime);
  digitalWrite(PIN_LED2, LOW);
  delay(blinkTime);
  digitalWrite(PIN_LED2, HIGH);
  delay(blinkTime);
}
