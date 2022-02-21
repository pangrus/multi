/*
                   _ _   _
                  | | | (_)
   _ __ ___  _   _| | |_ _
  | '_ ` _ \| | | | | __| |
  | | | | | | |_| | | |_| |
  |_| |_| |_|\__,_|_|\__|_|

  default software V1.0

  midi controller with 6 MIDI control change
  6 oscillators drone generator with 6 lfos

  -------------------------------------------
  Knobs 1 to 6   - oscillator frequency and midi controller
  Pb1           - drone on/off
  Pb2           - lfos on/off

  CC BY-NC-SA
  pangrus 2021
*/

// mozzi library
#include <MozziGuts.h>
// needed for mtof
#include <mozzi_midi.h>
// oscillators template
#include <Oscil.h>
// wavetable
#include <tables/triangle2048_int8.h>
// midi stuff
#include <MIDI.h>
#include <Adafruit_TinyUSB.h>   //tested on version 0.10.5

// assign CC to knobs
#define CC_KNOB1 73     // attack
#define CC_KNOB2 80     // decay
#define CC_KNOB3 64     // sustain
#define CC_KNOB4 72     // release
#define CC_KNOB5 74     // cutoff
#define CC_KNOB6 71     // resonance

// 6 oscillators
Oscil <TRIANGLE2048_NUM_CELLS, AUDIO_RATE> oscillator0(TRIANGLE2048_DATA);
Oscil <TRIANGLE2048_NUM_CELLS, AUDIO_RATE> oscillator1(TRIANGLE2048_DATA);
Oscil <TRIANGLE2048_NUM_CELLS, AUDIO_RATE> oscillator2(TRIANGLE2048_DATA);
Oscil <TRIANGLE2048_NUM_CELLS, AUDIO_RATE> oscillator3(TRIANGLE2048_DATA);
Oscil <TRIANGLE2048_NUM_CELLS, AUDIO_RATE> oscillator4(TRIANGLE2048_DATA);
Oscil <TRIANGLE2048_NUM_CELLS, AUDIO_RATE> oscillator5(TRIANGLE2048_DATA);

// 6 lfos
Oscil<TRIANGLE2048_NUM_CELLS, CONTROL_RATE> lfo0(TRIANGLE2048_DATA);
Oscil<TRIANGLE2048_NUM_CELLS, CONTROL_RATE> lfo1(TRIANGLE2048_DATA);
Oscil<TRIANGLE2048_NUM_CELLS, CONTROL_RATE> lfo2(TRIANGLE2048_DATA);
Oscil<TRIANGLE2048_NUM_CELLS, CONTROL_RATE> lfo3(TRIANGLE2048_DATA);
Oscil<TRIANGLE2048_NUM_CELLS, CONTROL_RATE> lfo4(TRIANGLE2048_DATA);
Oscil<TRIANGLE2048_NUM_CELLS, CONTROL_RATE> lfo5(TRIANGLE2048_DATA);

// sound generation variables
byte midiNote[6];
byte amplitude0, amplitude1, amplitude2, amplitude3, amplitude4, amplitude5;
int freq[6];
float lfoFreq0, lfoFreq1, lfoFreq2, lfoFreq3, lfoFreq4, lfoFreq5;
long out;

// knobs management variables
int actualKnob[] = {0, 0, 0, 0, 0, 0};
int storedKnob[] = {0, 0, 0, 0, 0, 0};
int thresholdValue = 12;

// pushbuttons management variables
byte pb1Pin = 9;
byte pb2Pin = 10;
bool pb1State;
bool pb2State;
bool lastPb1State = LOW;
bool lastPb2State = LOW;
bool modePb1 = HIGH;
bool modePb2 = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 20;

// usb midi
Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI_USB);

// din midi
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI_DIN);

// use #define for CONTROL_RATE, not a constant
#define CONTROL_RATE 64 // Hz, powers of 2 are most reliable

void setup() {
  MIDI_USB.begin(MIDI_CHANNEL_OMNI);
  MIDI_DIN.begin(MIDI_CHANNEL_OMNI);
  MIDI_DIN.turnThruOff();
  // ADC resolution
  analogReadResolution(9);
  analogWriteResolution(10);
  startMozzi(CONTROL_RATE);
  pinMode(pb1Pin, INPUT_PULLUP);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(pb2Pin, INPUT_PULLUP);
  pinMode(PIN_LED3, OUTPUT);
  // lfo frequencies
  lfo0.setFreq(0.0161f);
  lfo1.setFreq(0.0253f);
  lfo2.setFreq(0.0344f);
  lfo3.setFreq(0.0435f);
  lfo4.setFreq(0.0526f);
  lfo5.setFreq(0.0617f);
}

void updateControl() {
  MIDI_USB.read();

  // Pb1 management
  bool readPb1 = digitalRead(pb1Pin);
  if (readPb1 != lastPb1State) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (readPb1 != pb1State) {
      pb1State = readPb1;
      if (pb1State == LOW) {
        modePb1 = !modePb1;
        digitalWrite (PIN_LED3, modePb1);
      }
    }
  }
  lastPb1State = readPb1;

  // Pb2 management
  bool readPb2 = digitalRead(pb2Pin);
  if (readPb2 != lastPb2State) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (readPb2 != pb2State) {
      pb2State = readPb2;
      if (pb2State == LOW) {
        modePb2 = !modePb2;
        digitalWrite (PIN_LED2, modePb2);
      }
    }
  }
  lastPb2State = readPb2;

  // lfo
  if (!modePb2) {
    amplitude0 = map (lfo0.next(), -127, 127, 2, 253);
    amplitude1 = map (lfo1.next(), -127, 127, 2, 253);
    amplitude2 = map (lfo2.next(), -127, 127, 2, 253);
    amplitude3 = map (lfo3.next(), -127, 127, 2, 253);
    amplitude4 = map (lfo4.next(), -127, 127, 2, 253);
    amplitude5 = map (lfo5.next(), -127, 127, 2, 253);
  }
  else {
    amplitude0 = amplitude1 = amplitude2 = amplitude3 = amplitude4 = amplitude5 = 200;
  }

  // read pots
  actualKnob[0] = analogRead(1);
  actualKnob[1] = analogRead(2);
  actualKnob[2] = analogRead(3);
  actualKnob[3] = analogRead(4);
  actualKnob[4] = analogRead(5);
  actualKnob[5] = analogRead(8);

  for (int i = 0; i < 6; i++) {
    // if knobs have been moved generate midi cc
    if (abs(actualKnob[i] - storedKnob[i]) > thresholdValue) {

      // send MIDI CC
      switch (i) {
        case 0:
          MIDI_USB.sendControlChange(CC_KNOB1, actualKnob[i] >> 2, 1);
          MIDI_DIN.sendControlChange(CC_KNOB1, actualKnob[i] >> 2, 1);
          break;
        case 1:
          MIDI_USB.sendControlChange(CC_KNOB2, actualKnob[i] >> 2, 1);
          MIDI_DIN.sendControlChange(CC_KNOB2, actualKnob[i] >> 2, 1);
          break;
        case 2:
          MIDI_USB.sendControlChange(CC_KNOB3, actualKnob[i] >> 2, 1);
          MIDI_DIN.sendControlChange(CC_KNOB3, actualKnob[i] >> 2, 1);
          break;
        case 3:
          MIDI_USB.sendControlChange(CC_KNOB4, actualKnob[i] >> 2, 1);
          MIDI_DIN.sendControlChange(CC_KNOB4, actualKnob[i] >> 2, 1);
          break;
        case 4:
          MIDI_USB.sendControlChange(CC_KNOB5, actualKnob[i] >> 2, 1);
          MIDI_DIN.sendControlChange(CC_KNOB5, actualKnob[i] >> 2, 1);
          break;
        case 5:
          MIDI_USB.sendControlChange(CC_KNOB6, actualKnob[i] >> 2, 1);
          MIDI_DIN.sendControlChange(CC_KNOB6, actualKnob[i] >> 2, 1);
          break;
      }
      storedKnob[i] = actualKnob[i];
      
      // drone
      midiNote[i] = map (storedKnob[i], 0, 511, 36, 72);
      freq[i] = mtof(midiNote[i]);
    }
  }
  
  // set the oscillators frequencies
  oscillator0.setFreq(freq[0]);
  oscillator1.setFreq(freq[1]);
  oscillator2.setFreq(freq[2]);
  oscillator3.setFreq(freq[3]);
  oscillator4.setFreq(freq[4]);
  oscillator5.setFreq(freq[5]);
}

int updateAudio() {
  if (!modePb1) {
    out = ( oscillator0.next() * amplitude0 +
            oscillator1.next() * amplitude1 +
            oscillator2.next() * amplitude2 +
            oscillator3.next() * amplitude3 +
            oscillator4.next() * amplitude4 +
            oscillator5.next() * amplitude5 ) >> 9;
  }
  else {
    out = 0;
  }
  return out;
}

void loop() {
  audioHook();
}
