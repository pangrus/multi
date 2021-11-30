/*
                   _ _   _
                  | | | (_)
   _ __ ___  _   _| | |_ _
  | '_ ` _ \| | | | | __| |
  | | | | | | |_| | | |_| |
  |_| |_| |_|\__,_|_|\__|_|

  default software

  6 midi cc + 6 oscillators drone with 6 lfos
  -------------------------------------------
  KNOB 1 to 6   - oscillator frequency and midi controller
  PB1           - drone on/off
  PB2           - lfos on/off

  pangrus 2021
*/

// mozzi library
#include <MozziGuts.h>
// needed for mtof
#include <mozzi_midi.h>
// oscillators template
#include <Oscil.h>
// wavetable
#include <tables/sin2048_int8.h>
// midi stuff
#include <MIDI.h>
#include <Adafruit_TinyUSB.h>   //tested on version 0.10.5

// 6 oscillators with sinusoidal waveform
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> Sin0(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> Sin1(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> Sin2(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> Sin3(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> Sin4(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> Sin5(SIN2048_DATA);

// 6 lfos
Oscil<SIN2048_NUM_CELLS, CONTROL_RATE> lfo0(SIN2048_DATA);
Oscil<SIN2048_NUM_CELLS, CONTROL_RATE> lfo1(SIN2048_DATA);
Oscil<SIN2048_NUM_CELLS, CONTROL_RATE> lfo2(SIN2048_DATA);
Oscil<SIN2048_NUM_CELLS, CONTROL_RATE> lfo3(SIN2048_DATA);
Oscil<SIN2048_NUM_CELLS, CONTROL_RATE> lfo4(SIN2048_DATA);
Oscil<SIN2048_NUM_CELLS, CONTROL_RATE> lfo5(SIN2048_DATA);

// variables
int actualKnob[] = {0, 0, 0, 0, 0, 0};
int storedKnob[] = {0, 0, 0, 0, 0, 0};
int thresholdValue = 12;
long out;
int freq[6];
byte midiNote[6];
byte PB1Pin = 9;
byte PB1State;
byte PB2Pin = 10;
byte PB2State;
bool lastPB1State = LOW;
bool modePB1 = HIGH;
bool lastPB2State = LOW;
bool modePB2 = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 20;
float lfoFreq0, lfoFreq1, lfoFreq2, lfoFreq3, lfoFreq4, lfoFreq5;
byte amplitude0, amplitude1, amplitude2, amplitude3, amplitude4, amplitude5;

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
  analogReadResolution(8);
  startMozzi(CONTROL_RATE);
  pinMode(PB1Pin, INPUT_PULLUP);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PB2Pin, INPUT_PULLUP);
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

  // PB1 management - drone on-off
  bool readPB1 = digitalRead(PB1Pin);
  if (readPB1 != lastPB1State) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (readPB1 != PB1State) {
      PB1State = readPB1;
      if (PB1State == LOW) {
        modePB1 = !modePB1;
        digitalWrite (PIN_LED3, modePB1);
      }
    }
  }
  lastPB1State = readPB1;

  // PB2 management - lfos on/off
  bool readPB2 = digitalRead(PB2Pin);
  if (readPB2 != lastPB2State) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (readPB2 != PB2State) {
      PB2State = readPB2;
      if (PB2State == LOW) {
        modePB2 = !modePB2;
        digitalWrite (PIN_LED2, modePB2);
      }
    }
  }
  lastPB2State = readPB2;

  if (!modePB2) {
    amplitude0 = lfo0.next();
    amplitude1 = lfo1.next();
    amplitude2 = lfo2.next();
    amplitude3 = lfo3.next();
    amplitude4 = lfo4.next();
    amplitude5 = lfo5.next();
  }
  else {
    amplitude0 = amplitude1 = amplitude2 = amplitude3 = amplitude4 = amplitude5 = 127;
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

      switch (i) {
        case 0:
          // attack
          MIDI_USB.sendControlChange(73, actualKnob[i] >> 1, 1);
          MIDI_DIN.sendControlChange(73, actualKnob[i] >> 1, 1);
          break;
        case 1:
          // decay
          MIDI_USB.sendControlChange(80, actualKnob[i] >> 1, 1);
          MIDI_DIN.sendControlChange(80, actualKnob[i] >> 1, 1);
          break;
        case 2:
          // sustain
          MIDI_USB.sendControlChange(64, actualKnob[i] >> 1, 1);
          MIDI_DIN.sendControlChange(64, actualKnob[i] >> 1, 1);
          break;
        case 3:
          // release
          MIDI_USB.sendControlChange(72, actualKnob[i] >> 1, 1);
          MIDI_DIN.sendControlChange(72, actualKnob[i] >> 1, 1);
          break;
        case 4:
          // cutoff
          MIDI_USB.sendControlChange(74, actualKnob[i] >> 1, 1);
          MIDI_DIN.sendControlChange(74, actualKnob[i] >> 1, 1);
          break;
        case 5:
          // resonance
          MIDI_USB.sendControlChange(71, actualKnob[i] >> 1, 1);
          MIDI_DIN.sendControlChange(71, actualKnob[i] >> 1, 1);
          break;
      }
      storedKnob[i] = actualKnob[i];
    }
    // tuned drone
    midiNote[i] = map (actualKnob[i], 0, 256, 48, 84);
    freq[i] = mtof(midiNote[i]);
  }
  // set the oscillators frequencies
  Sin0.setFreq(freq[0]);
  Sin1.setFreq(freq[1]);
  Sin2.setFreq(freq[2]);
  Sin3.setFreq(freq[3]);
  Sin4.setFreq(freq[4]);
  Sin5.setFreq(freq[5]);
}

int updateAudio() {
  if (!modePB1) {
    out = (
            Sin0.next() * amplitude0 +
            Sin1.next() * amplitude1 +
            Sin2.next() * amplitude2 +
            Sin3.next() * amplitude3 +
            Sin4.next() * amplitude4 +
            Sin5.next() * amplitude5 ) >> 9;
  }
  else {
    out = 0;
  }
  return out;
}

void loop() {
  audioHook();
}
