/*
                   _ _   _
                  | | | (_)
   _ __ ___  _   _| | |_ _
  | '_ ` _ \| | | | | __| |
  | | | | | | |_| | | |_| |
  |_| |_| |_|\__,_|_|\__|_|

  hardware test V1.0
  ------------------
  pb1 toggles led3
  pb2 toggles led2
  led1 blinks accordingly to the last moved knob
  knobs values and pushbutton states are displayed in the serial monitor

  CC BY-NC-SA
  pangrus 2021
*/

// knob variables
byte actualKnob[] = {0, 0, 0, 0, 0, 0, 0};
byte storedKnob[] = {0, 0, 0, 0, 0, 0, 0};
byte knobThreshold = 5;
byte selectedKnob;

// pushbutton variables
byte pb1Pin = 9;
byte pb2Pin = 10;
bool readPb1;
bool readPb2;
bool pb1State;
bool pb2State;
bool pb1Mode;
bool pb2Mode;
bool lastPb1State;
bool lastPb2State;
long lastDebounceTime = 0;
long debounceDelay = 50;

// blink variables
bool ledState;
byte blinks;
unsigned long currentMillis = 0;
unsigned long previousMillis = 0;
unsigned long blinkTime = 200;

void setup() {
  SerialUSB.begin(115200); // USB serial
  analogReadResolution(8); // ADC resolution (up to 12 bit)
  pinMode(pb1Pin, INPUT_PULLUP);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(pb2Pin, INPUT_PULLUP);
  pinMode(PIN_LED3, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  manageKnobs();
  managePushbuttons();
  manageBlinks();
}

void managePushbuttons() {
  // pb1 management
  readPb1 = digitalRead(pb1Pin);
  if (readPb1 != lastPb1State) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (readPb1 != pb1State) {
      pb1State = readPb1;
      if (pb1State == LOW) {
        pb1Mode = !pb1Mode;
        if (pb1Mode) Serial.println ("PB1 MODE OFF");
        else Serial.println ("PB1 MODE ON");
        digitalWrite (PIN_LED3, !pb1Mode);
      }
    }
  }
  lastPb1State = readPb1;
  // pb2 management
  readPb2 = digitalRead(pb2Pin);
  if (readPb2 != lastPb2State) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (readPb2 != pb2State) {
      pb2State = readPb2;
      if (pb2State == LOW) {
        pb2Mode = !pb2Mode;
        if (pb2Mode) Serial.println ("PB2 MODE OFF");
        else Serial.println ("PB2 MODE ON");
        digitalWrite (PIN_LED2, !pb2Mode);
      }
    }
  }
  lastPb2State = readPb2;
}

void manageKnobs() {
  actualKnob[0] = analogRead(1);
  actualKnob[1] = analogRead(2);
  actualKnob[2] = analogRead(3);
  actualKnob[3] = analogRead(4);
  actualKnob[4] = analogRead(5);
  actualKnob[5] = analogRead(8);
  for (int i = 0; i < 6; i++) {
    if (abs(actualKnob[i] - storedKnob[i]) > knobThreshold) {
      storedKnob[i] = actualKnob[i];
      selectedKnob = i + 1;
      Serial.print("KNOB ");
      Serial.print(selectedKnob);
      Serial.print(" = ");
      Serial.println(storedKnob[i]);
    }
  }
}

void manageBlinks() {
  currentMillis = millis();
  if (currentMillis - previousMillis >= blinkTime) {
    previousMillis = currentMillis;
    if (ledState == LOW) ledState = HIGH;
    else {
      ledState = LOW;
      blinks++;
      if (blinks > selectedKnob) {
        blinks = 0;
        ledState = HIGH;
      }
    }
    digitalWrite(LED_BUILTIN, ledState);
  }
}
