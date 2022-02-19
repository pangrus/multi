/*
                   _ _   _
                  | | | (_)
   _ __ ___  _   _| | |_ _
  | '_ ` _ \| | | | | __| |
  | | | | | | |_| | | |_| |
  |_| |_| |_|\__,_|_|\__|_|

   euclidean sequencer v0.19
   -------------------------
   receives midi clock, start/stop on the MIDI DIN input 
   generates euclidean rhythmes on the MIDI DIN output
   knob 1,2,3 selects number of beats to be played
   knob 4,5,6 pattern length
   pb1 randomize pattern 2
   pb2 randomize pattern 3

   CC BY-NC-SA
   pangrus 2022
*/

#include <MIDI.h>

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI_DIN);

// MIDI notes
byte midiNote0 = 60;  // C3
byte midiNote1 = 62;  // D3
byte midiNote2 = 64;  // E3

// sequencer variables
bool isStarted = 0;
char pattern[3][16];
char patternSwap[16];
byte clockCounter = 0;
byte beats1 = 0;
byte beats2 = 0;
byte beats3 = 0;
byte length1 = 16;
byte length2 = 16;
byte length3 = 16;
byte activeStep1 = 0;
byte activeStep2 = 0;
byte activeStep3 = 0;

// pushbutton variables
byte pb1State;
bool lastPb1State = HIGH;
bool pb1Mode = LOW;
bool readPb1;
byte pb2State;
bool lastPb2State = HIGH;
bool pb2Mode = LOW;
bool readPb2;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 20;

// knobs are debounced and values are stored
byte actualKnob[] = {0, 0, 0, 0, 0, 0, 0};
byte storedKnob[] = {0, 0, 0, 0, 0, 0, 0};
byte knobThreshold = 5;
byte selectedKnob;

void setup() {
  SerialUSB.begin(11250);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_LED3, OUTPUT);
  MIDI_DIN.begin(MIDI_CHANNEL_OMNI);
  MIDI_DIN.setHandleStart(HandleStart);
  MIDI_DIN.setHandleStop(HandleStop);
  MIDI_DIN.setHandleClock(HandleClock);
  MIDI_DIN.turnThruOff();
  analogReadResolution(7);
  for (int i = 0; i < 15; i++) {
    patternSwap[i] = '.';
  }
}

void loop() {
  MIDI_DIN.read();
  manageKnobs();
  managePushbuttons();

  beats1 = map (storedKnob[0], 0, 120, 0, 16);
  beats2 = map (storedKnob[1], 0, 120, 0, 16);
  beats3 = map (storedKnob[2], 0, 120, 0, 16);
  length1 = map (storedKnob[3], 0, 120, 1, 16);
  length2 = map (storedKnob[4], 0, 120, 1, 16);
  length3 = map (storedKnob[5], 0, 120, 1, 16);

  createPattern(0, beats1, length1);
  createPattern(1, beats2, length2);
  createPattern(2, beats3, length3);

  // random
  if (pb1Mode) createPattern(1, random (beats2), 16);
  if (pb2Mode) createPattern(2, random (beats3), 16);

  rotatePattern(1, 4);
  rotatePattern(2, 2);
}

void HandleStart() {
  SerialUSB.println("MIDI Start");
  clockCounter = 0;
  activeStep1 = 0;
  activeStep2 = 0;
  activeStep3 = 0;
  isStarted = 1;
}

void HandleStop() {
  SerialUSB.println("MIDI Stop");
  isStarted = 0;
}

void HandleClock() {
  if (isStarted) {
    MIDI_DIN.sendNoteOff (midiNote2, 0, 10);
    if (clockCounter == 0) {
      if (pattern[0][activeStep1] == 'x') MIDI_DIN.sendNoteOn (midiNote0, 127, 10);
      if (pattern[1][activeStep2] == 'x') MIDI_DIN.sendNoteOn (midiNote1, 127, 10);
      if (pattern[2][activeStep3] == 'x') MIDI_DIN.sendNoteOn (midiNote2, 127, 10);
      activeStep1++;
      activeStep2++;
      activeStep3++;
      if (activeStep1 > length1 - 1) activeStep1 = 0;
      if (activeStep2 > length2 - 1) activeStep2 = 0;
      if (activeStep3 > length3 - 1) activeStep3 = 0;
      MIDI_DIN.sendNoteOff (midiNote0, 0, 10);
      MIDI_DIN.sendNoteOff (midiNote1, 0, 10);
      MIDI_DIN.sendNoteOff (midiNote2, 0, 10);
    }
    if (clockCounter++ > 4) clockCounter = 0;
    digitalWrite (LED_BUILTIN, clockCounter);
  }
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
/*     
      SerialUSB.print ("Knob ");
      SerialUSB.print (i + 1);
      SerialUSB.print (" value :");
      SerialUSB.print (storedKnob[i]);
      SerialUSB.println("--");
      SerialUSB.println(beats1);
      SerialUSB.println(beats2);
      SerialUSB.println(beats3);
      SerialUSB.println(length1);
      SerialUSB.println(length2);
      SerialUSB.println(length3);
      SerialUSB.println("--");
*/

    }
  }
}

void managePushbuttons() {
  // pb1
  readPb1 = digitalRead(9);
  if (readPb1 != lastPb1State) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (readPb1 != pb1State) {
      pb1State = readPb1;
      if (pb1State == LOW) {
        pb1Mode = !pb1Mode;
        digitalWrite (PIN_LED3, !pb1Mode);
      }
    }
  }
  lastPb1State = readPb1;
  // pb2
  readPb2 = digitalRead(10);
  if (readPb2 != lastPb2State) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (readPb2 != pb2State) {
      pb2State = readPb2;
      if (pb2State == LOW) {
        pb2Mode = !pb2Mode;
        digitalWrite (PIN_LED2, !pb2Mode);
      }
    }
  }
  lastPb2State = readPb2;
}

void createPattern(byte patternNumber, byte colpi, byte lengthPattern) {
  int cur = lengthPattern;
  for (int i = 0; i < lengthPattern; i++) {
    pattern[patternNumber][i] = '.';
    if (cur >= lengthPattern) {
      cur -= lengthPattern;
      pattern[patternNumber][i] = 'x';
    }
    if (colpi == 0) pattern[patternNumber][i] = '.';
    cur += colpi;
  }
}

void rotatePattern(byte patternNumber, byte rotateAmount) {
  for (int i = 0; i < 16; i++) {
    patternSwap[i] = pattern[patternNumber][i];
  }
  for (int i = 0; i < 16; i++) {
    if  (i < rotateAmount) pattern[patternNumber][i] = patternSwap[i + 16 - rotateAmount];
    else pattern[patternNumber][i] = patternSwap[i - rotateAmount];
  }
}
