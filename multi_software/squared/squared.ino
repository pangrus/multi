/*
                                      _
   ___  __ _ _   _  __ _ _ __ ___  __| |
  / __|/ _` | | | |/ _` | '__/ _ \/ _` |
  \__ \ (_| | |_| | (_| | | |  __/ (_| |
  |___/\__, |\__,_|\__,_|_|  \___|\__,_|
          |_|

  CC BY-NC-SA pangrus 2023
  ------------------------

  5 squarewave oscillators with 5 squarewave lfos
  --------------------------------------------------
  knobs 1 to 5  controls the oscillator frequency and sends MIDI CC on USB and DIN connectors
  knob 6 control the lfos speed
  pb1 switches  drone on/off
  pb2 switches  lfos on/off

*/

#include <MozziGuts.h>
#include <Oscil.h>
#include <mozzi_rand.h>
#include <mozzi_midi.h>
#include <MIDI.h>
#include <Adafruit_TinyUSB.h>   // tested with version 0.10.5 

// wavetables
#include <tables/square_no_alias512_int8.h>
#include <tables/saw256_int8.h>

// oscillators
Oscil <SQUARE_NO_ALIAS512_NUM_CELLS, AUDIO_RATE> oscillator[6];
// lfos
Oscil<SQUARE_NO_ALIAS512_NUM_CELLS, CONTROL_RATE> lfo[6];

// MIDI
Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI_USB);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI_DIN);

// MIDI variables
byte usbChannel = 1;
byte dinChannel = 2;

// assign MIDI CC to knobs
#define CC_KNOB1 73     // attack
#define CC_KNOB2 80     // decay
#define CC_KNOB3 64     // sustain
#define CC_KNOB4 72     // release
#define CC_KNOB5 74     // cutoff
#define CC_KNOB6 71     // resonance

// knobs are debounced and values are stored
byte actualKnob[] = {0, 0, 0, 0, 0, 0, 0};
byte storedKnob[] = {0, 0, 0, 0, 0, 0, 0};
byte knobThreshold = 3;
byte selectedKnob;

// pushbuttons management variables
bool pb1Mode;
bool pb2Mode;
bool readPb1;
bool readPb2;
bool pb1State;
bool pb2State;
bool lastPb1State;
bool lastPb2State;
bool isStarted = LOW;
bool normalSequencing = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 30000;

// drone variables
byte midiNote[6];
byte level[6];
int freq[6];
float lfoFreq[6];
long squaredOut;

void setup() {
  startMozzi();
  analogReadResolution (7);
  analogWriteResolution (10);
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_LED3, OUTPUT);
  SerialUSB.begin(115200);
  MIDI_USB.begin(usbChannel);
  MIDI_DIN.begin(dinChannel);
  MIDI_DIN.turnThruOff();
  storedKnob[0] = analogRead(1);
  storedKnob[1] = analogRead(2);
  storedKnob[2] = analogRead(3);
  storedKnob[3] = analogRead(4);
  storedKnob[4] = analogRead(5);
  storedKnob[5] = analogRead(8);
  for ( int i = 0; i < 6; i++) {
    oscillator[i].setTable(SQUARE_NO_ALIAS512_DATA);
    lfo[i].setTable(SQUARE_NO_ALIAS512_DATA);
  }
  lfo[0].setFreq(0.131f);
  lfo[1].setFreq(0.181f);
  lfo[2].setFreq(0.239f);
  lfo[3].setFreq(0.421f);
  lfo[4].setFreq(0.557f);
  lfo[5].setFreq(0.673f);
}

void loop() {
  audioHook();
  MIDI_USB.read();
  MIDI_DIN.read();
}

void updateControl() {
  managePushbuttons();
  manageKnobs();
  squared();
}

void squared() {
  for (int i = 0; i < 5; i++) {
    midiNote[i] = map (storedKnob[i], 0, 120, 36, 72);
    freq[i] = mtof(midiNote[i]);
    oscillator[i].setFreq(freq[i]);
    lfo[0].setFreq(0.0131f * storedKnob[5]);
    lfo[1].setFreq(0.0181f * storedKnob[5]);
    lfo[2].setFreq(0.0239f * storedKnob[5]);
    lfo[3].setFreq(0.0421f * storedKnob[5]);
    lfo[4].setFreq(0.0557f * storedKnob[5]);
    lfo[5].setFreq(0.0673f * storedKnob[5]);
    if (pb2Mode) level[i] = map (lfo[i].next(), -127, 127, 10, 240);
    else level[i] = 200;
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
      // send MIDI CC
      switch (i) {
        case 0:
          MIDI_USB.sendControlChange(CC_KNOB1, actualKnob[i], usbChannel);
          MIDI_DIN.sendControlChange(CC_KNOB1, actualKnob[i], dinChannel);
          break;
        case 1:
          MIDI_USB.sendControlChange(CC_KNOB2, actualKnob[i], usbChannel);
          MIDI_DIN.sendControlChange(CC_KNOB2, actualKnob[i], dinChannel);
          break;
        case 2:
          MIDI_USB.sendControlChange(CC_KNOB3, actualKnob[i], usbChannel);
          MIDI_DIN.sendControlChange(CC_KNOB3, actualKnob[i], dinChannel);
          break;
        case 3:
          MIDI_USB.sendControlChange(CC_KNOB4, actualKnob[i], usbChannel);
          MIDI_DIN.sendControlChange(CC_KNOB4, actualKnob[i], dinChannel);
          break;
        case 4:
          MIDI_USB.sendControlChange(CC_KNOB5, actualKnob[i], usbChannel);
          MIDI_DIN.sendControlChange(CC_KNOB5, actualKnob[i], dinChannel);
          break;
        case 5:
          MIDI_USB.sendControlChange(CC_KNOB6, actualKnob[i], usbChannel);
          MIDI_DIN.sendControlChange(CC_KNOB6, actualKnob[i], dinChannel);
          break;
      }
      storedKnob[i] = actualKnob[i];
      selectedKnob = i + 1;
      SerialUSB.print ("Knob ");
      SerialUSB.print (selectedKnob);
      SerialUSB.print (" value :");
      SerialUSB.println (storedKnob[i]);
    }
  }
}

void managePushbuttons() {
  // pb1
  readPb1 = digitalRead(9);
  if (readPb1 != lastPb1State) {
    lastDebounceTime = mozziMicros();
  }
  if ((mozziMicros() - lastDebounceTime) > debounceDelay) {
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
    lastDebounceTime = mozziMicros();
  }
  if ((mozziMicros() - lastDebounceTime) > debounceDelay) {
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

AudioOutput_t updateAudio() {
  if (pb1Mode) {
    for (int i = 0; i < 5; i++) {
      squaredOut += (oscillator[i].next() * level[i]);
    }
  }
  else {
    squaredOut = 0;
  }
  squaredOut = squaredOut >> 9;
  return squaredOut;
}
