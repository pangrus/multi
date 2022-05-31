/*
    __                  _
   / _| ___  _   _ _ __(_) ___ _ __
  | |_ / _ \| | | | '__| |/ _ \ '__|
  |  _| (_) | |_| | |  | |  __/ |
  |_|  \___/ \__,_|_|  |_|\___|_|

  fourier v0.4
  CC BY-NC-SA pangrus 2022
  ------------------------
  3 voices drone generator
  each voice is composed by six oscillators tuned on the harmonic overtones
  each voice has six lfos, one for each oscillator level

  KNOB1       controls the first voice main frequency
  KNOB2       controls the second voice main frequency
  KNOB3       controls the third voice main frequency
  PB1         drone on/off
  PB2         lfo mode on/off

  --lfo mode off--
  KNOB4       changes first voice timbre by affecting the overtones level
  KNOB5       changes second voice timbre by affecting the overtones level
  KNOB6       changes third voice timbre by affecting the overtones level

  --lfo mode on--
  KNOB4       changes the six lfos speeds of the first voice
  KNOB5       changes the six lfos speeds of the second voice
  KNOB6       changes the six lfos speeds of the third voice

  knobs 1 to 6 are sending MIDI CC on USB and DIN connectors
*/

#include <MozziGuts.h>
#include <Oscil.h>
#include <mozzi_rand.h>
#include <mozzi_midi.h>
#include <MIDI.h>
#include <Adafruit_TinyUSB.h>

// wavetable
#include <tables/sin4096_int8.h>

// MIDI
Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI_USB);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI_DIN);
byte usbChannel = 1;
byte dinChannel = 1;

// assign here the MIDI CC to knobs
#define CC_KNOB1 73 // attack
#define CC_KNOB2 80 // decay
#define CC_KNOB3 64 // sustain
#define CC_KNOB4 72 // release
#define CC_KNOB5 74 // cutoff
#define CC_KNOB6 71 // resonance

// knobs are debounced and values are stored
int actualKnob[] = {0, 0, 0, 0, 0, 0, 0};
int storedKnob[] = {0, 0, 0, 0, 0, 0, 0};
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

// sound generation variables
Oscil < SIN4096_NUM_CELLS, AUDIO_RATE > fourierOsc[3][6];
Oscil < SIN4096_NUM_CELLS, CONTROL_RATE > fourierLfo[3][6];
byte level[3][6];
int fourierFreq[3];
long fourierOut;

void setup() {
  startMozzi();
  analogReadResolution(7);
  analogWriteResolution(10);
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_LED3, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
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
  for (int j = 0; j < 3; j++) {
    for (int i = 0; i < 6; i++) {
      fourierOsc[j][i].setTable(SIN4096_DATA);
      fourierLfo[j][i].setTable(SIN4096_DATA);
    }
  }
}

void loop() {
  audioHook();
  MIDI_USB.read();
  MIDI_DIN.read();
}

void updateControl() {
  managePushbuttons();
  manageKnobs();
  fourier();
}

void fourier() {
  for (int j = 0; j < 3; j++) {
    if (!pb2Mode) {
      level[j][0] = storedKnob[j + 3] << 1;
      level[j][1] = storedKnob[j + 3] << 1;
      level[j][2] = 127 - storedKnob[j + 3] << 1;
      level[j][3] = storedKnob[j + 3] << 1;
      level[j][4] = 127 - storedKnob[j + 3] << 1;
      level[j][5] = storedKnob[j + 3] << 1;
    }
    else {
      for (int i = 1; i < 6; i++) {
        fourierLfo[j][i].setFreq((float)storedKnob[j + 3] * (j + 1) / (i * 200));
        level[j][i] = map(fourierLfo[j][i].next(), -127, 127, 3, 247);
      }
    }
    // choose starting chord
    // mtof converts the midi note number to the corresponding frequency
    fourierFreq[0] = mtof((storedKnob[0] >> 2) + 39);
    fourierFreq[1] = mtof((storedKnob[1] >> 2) + 44);
    fourierFreq[2] = mtof((storedKnob[2] >> 2) + 56);
    fourierOsc[j][0].setFreq(fourierFreq[j]);
    for (int i = 1; i < 6; i++) {
      fourierOsc[j][i].setFreq(fourierFreq[j] * (i + 1));
    }
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
      SerialUSB.print("Knob ");
      SerialUSB.print(selectedKnob);
      SerialUSB.print(" value :");
      SerialUSB.println(storedKnob[i]);
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
        digitalWrite(PIN_LED3, !pb1Mode);
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
        digitalWrite(PIN_LED2, !pb2Mode);
      }
    }
  }
  lastPb2State = readPb2;
}

AudioOutput_t updateAudio() {
  if (pb1Mode) {
    for (int j = 0; j < 3; j++) {
      for (int i = 0; i < 6; i++) {
        fourierOut += fourierOsc[j][i].next() * level[j][i];
      }
    }
  }
  else {
    fourierOut = 0;
  }
  fourierOut = fourierOut >> 10;
  return fourierOut;
}
