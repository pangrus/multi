/*
                   _ _   _
                  | | | (_)
   _ __ ___  _   _| | |_ _
  | '_ ` _ \| | | | | __| |
  | | | | | | |_| | | |_| |
  |_| |_| |_|\__,_|_|\__|_|

  hardware test

  CC BY-NC-SA
  pangrus 2021
*/

byte knob[] =  {0, 0, 0, 0, 0, 0, 0};
byte oldknob[] = {0, 0, 0, 0, 0, 0, 0};
byte knobThreshold = 5;
byte pb1Pin = 9;
byte pb2Pin = 10;
bool readPb1;
bool readPb2;
bool pb1State;
bool pb2State;
bool pb1Mode = HIGH;
bool pb2Mode = HIGH;
bool lastPb1State = LOW;
bool lastPb2State = LOW;
long lastDebounceTime = 0;
long debounceDelay = 50;

void setup() {
  // USB serial
  SerialUSB.begin(115200);
  // ADC resolution (up to 12 bit)
  analogReadResolution(8);
  pinMode(pb1Pin, INPUT_PULLUP);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(pb2Pin, INPUT_PULLUP);
  pinMode(PIN_LED3, OUTPUT);
}

void loop() {
  
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
        digitalWrite (PIN_LED3, pb1Mode);
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
        digitalWrite (PIN_LED2, pb2Mode);
      }
    }
  }
  lastPb2State = readPb2;
  
  // read knobs
  knob[1] = analogRead(1);
  knob[2] = analogRead(2);
  knob[3] = analogRead(3);
  knob[4] = analogRead(4);
  knob[5] = analogRead(5);
  knob[6] = analogRead(8);
  for (int i = 1; i < 7; i++) {
    if (abs(knob[i] - oldknob[i]) > knobThreshold) {
      Serial.print("KNOB ");
      Serial.print(i);
      Serial.print(" = ");
      Serial.println(knob[i]);
      oldknob[i] = knob[i];
    }
  }
}
