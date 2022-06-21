/*
    __
   / _|_ __ ___
  | |_| '_ ` _ \
  |  _| | | | | |
  |_| |_| |_| |_|

  fm synth V1.0
  -------------

  PB1     hold note
  PB2     play note
  KNOB1   carrier frequency when not receiving MIDI through USB
  KNOB2   ratio
  KNOB3   modulation speed
  KNOB4   modulation amount
  KNOB5   attack
  KNOB6   release

  thanks to Peter Zimon for the inspiration
  and for https://www.peterzimon.com/zfm-mozzi-based-arduino-fm-synth/
  from where I copied part of the code

  CC-BY-NC-SA pangrus 2022
*/

#include <MozziGuts.h>
#include <Oscil.h>
#include <mozzi_midi.h>
#include <ADSR.h>
#include <MIDI.h>
#include <Smooth.h>
#include <Adafruit_TinyUSB.h>
#include <tables/sin1024_int8.h>

// MIDI
Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI_USB);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI_DIN);

// MIDI variables
byte usbChannel = 1;
byte dinChannel = 10;

// fm 
Oscil<SIN1024_NUM_CELLS, AUDIO_RATE> carrier, fmModulator (SIN1024_DATA);
Oscil<SIN1024_NUM_CELLS, AUDIO_RATE> fmModAmount (SIN1024_DATA);
ADSR <CONTROL_RATE, AUDIO_RATE> fmEnvelope;
Smooth <long> fmSmoothAmount(0.95);

// fm variables
int carrierFreq, fmModSpeed, fmModFrequency, fmNoteOn, fmButton, fmRatio, fmModAmountKnob, fmAttackTime, fmReleaseTime;
long fmAmount, fmMod;
bool fmPlaying;

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
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 30000;

// assign MIDI CC to knobs
#define CC_KNOB1 73     // attack
#define CC_KNOB2 80     // decay
#define CC_KNOB3 64     // sustain
#define CC_KNOB4 72     // release
#define CC_KNOB5 74     // cutoff
#define CC_KNOB6 71     // resonance

void setup() {
  analogReadResolution (7);
  analogWriteResolution (10);
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_LED3, OUTPUT);
  SerialUSB.begin(115200);
  MIDI_USB.begin(usbChannel);
  MIDI_USB.turnThruOff();
  MIDI_DIN.begin(dinChannel);
  MIDI_DIN.turnThruOff();
  MIDI_USB.setHandleNoteOn(HandleNoteOn);
  MIDI_USB.setHandleNoteOff(HandleNoteOff);
  startMozzi();
  fmEnvelope.setADLevels(255, 255);
  carrier.setTable(SIN1024_DATA);
  fmModulator.setTable(SIN1024_DATA);
  storedKnob[0] = analogRead(1);
  storedKnob[1] = analogRead(2);
  storedKnob[2] = analogRead(3);
  storedKnob[3] = analogRead(4);
  storedKnob[4] = analogRead(5);
  storedKnob[5] = analogRead(8);
}

void loop() {
  MIDI_USB.read();
  MIDI_DIN.read();
  audioHook();
}

void HandleNoteOn(byte channel, byte note, byte velocity) {
  if (fmButton == LOW) {
    return;
  }
  carrierFreq = mtof(note);
  carrier.setFreq(carrierFreq);
  fmEnvelope.noteOn();
  fmNoteOn++;
  digitalWrite (LED_BUILTIN,LOW);
}

void HandleNoteOff(byte channel, byte note, byte velocity) {
  fmNoteOn--;
  if (fmNoteOn <= 0) {
    fmEnvelope.noteOff();
    fmNoteOn = 0;
    digitalWrite (LED_BUILTIN,HIGH);
  }
}

void updateControl() {
  MIDI_USB.read();
  manageKnobs();
  managePushbuttons();
  fm();
}

void fm() {
  fmAttackTime = map (storedKnob[4], 0, 127, 1, 1000);
  fmReleaseTime = map (storedKnob[5], 0, 127, 10, 5000);
  fmEnvelope.setTimes(fmAttackTime, 255, 255, fmReleaseTime);
  fmEnvelope.update();
  fmButton = digitalRead(10);
  if (fmButton == LOW or pb1Mode) {
    fmPlaying = true;
    carrierFreq = map (analogRead(1), 0, 127, 32, 1000);
    carrier.setFreq(carrierFreq);
    fmEnvelope.noteOn();
    digitalWrite (PIN_LED3,LOW);
  }
  else {
    if (fmPlaying) {
      fmEnvelope.noteOff();
      fmPlaying = false;
      digitalWrite (PIN_LED3,HIGH);
    }
  }
  fmRatio = map (storedKnob[1], 0, 130, 2, 12);
  fmModFrequency = carrierFreq * fmRatio;
  fmModulator.setFreq(fmModFrequency);
  fmModAmountKnob = map (storedKnob[3], 0, 127, 0, 500);
  fmAmount = ((long)fmModAmountKnob * (fmModAmount.next() + 128)) >> 8;
  fmModSpeed = map (storedKnob[2], 0, 127, 0, 10000);
  fmModAmount.setFreq(fmModSpeed);
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
        //digitalWrite (PIN_LED2, !pb2Mode);
      }
    }
  }
  lastPb2State = readPb2;
}

int updateAudio() {
  fmMod = fmSmoothAmount.next(fmAmount) * fmModulator.next();
  return (fmEnvelope.next() * carrier.phMod(fmMod)) >> 8;
}
