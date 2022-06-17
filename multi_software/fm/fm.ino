/*
    __
   / _|_ __ ___
  | |_| '_ ` _ \
  |  _| | | | | |
  |_| |_| |_| |_|

  simple FM synth
  ---------------
  
  PB1     play note
  KNOB1   carrier frequency when not receiving MIDI through USB
  KNOB2   modulation speed
  KNOB3   modulation amount
  KNOB4   attack
  KNOB5   release
  KNOB6   modulator ratio

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
#include <tables/sin2048_int8.h>

// MIDI
Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

Oscil<SIN2048_NUM_CELLS, AUDIO_RATE> carrier, modulator, modulationAmount (SIN2048_DATA);
ADSR <CONTROL_RATE, AUDIO_RATE> envelope;
Smooth <long> aSmoothAmount(0.95);

int freq, modulationSpeed, modulationFrequency, notesPlaying, playButton, ratio;
long fmAmount;
bool play;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(9, INPUT_PULLUP);
  MIDI.begin(1);
  MIDI.setHandleNoteOn(HandleNoteOn);
  MIDI.setHandleNoteOff(HandleNoteOff);
  envelope.setADLevels(255, 255);
  startMozzi(CONTROL_RATE);
  carrier.setTable(SIN2048_DATA);
  modulator.setTable(SIN2048_DATA);
}

void loop() {
  audioHook();
}

void HandleNoteOn(byte channel, byte note, byte velocity) {
  if (playButton == LOW) {
    return;
  }
  freq = mtof(note);
  carrier.setFreq(freq);
  envelope.noteOn();
  digitalWrite(LED_BUILTIN, LOW);
  notesPlaying++;
}

void HandleNoteOff(byte channel, byte note, byte velocity) {
  notesPlaying--;
  if (notesPlaying <= 0) {
    envelope.noteOff();
    digitalWrite(LED_BUILTIN, HIGH);
    notesPlaying = 0;
  }
}

void updateControl() {
  MIDI.read();
  long attackTime = map (analogRead(4), 0, 1023, 1, 1000);
  long releaseTime = map (analogRead(5), 0, 1023, 10, 5000);
  envelope.setTimes(attackTime, 255, 255, releaseTime);
  envelope.update();
  playButton = digitalRead(9);
  if (playButton == LOW) {
    play = true;
    freq = map (mozziAnalogRead(A1), 0, 1023, 32, 1000);
    carrier.setFreq(freq);
    envelope.noteOn();
  }
  else {
    if (play) {
      envelope.noteOff();
      play = false;
    }
  }
  ratio = map (analogRead(A8), 0, 1030, 2, 8);
  modulationFrequency = freq * ratio;
  modulator.setFreq(modulationFrequency);
  int modulationAmountKnob = map (mozziAnalogRead(A3), 0, 1023, 0, 500);
  fmAmount = ((long)modulationAmountKnob * (modulationAmount.next() + 128)) >> 8;
  modulationSpeed = map (mozziAnalogRead(A2), 0, 1023, 0, 10000);
  modulationAmount.setFreq(modulationSpeed);
}

int updateAudio() {
  long modulation = aSmoothAmount.next(fmAmount) * modulator.next();
  return (envelope.next() * carrier.phMod(modulation)) >> 8;
}
