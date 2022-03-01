/*
                   _ _   _
                  | | | (_)
   _ __ ___  _   _| | |_ _
  | '_ ` _ \| | | | | __| |
  | | | | | | |_| | | |_| |
  |_| |_| |_|\__,_|_|\__|_|

  synth_sequencer V0.23
  ---------------------

  knob1 synth release
  knob2 synth cutoff
  knob3 synth resonance and detune, step advance amount
  knob4 selects sequence
  knob5 transpose sequence
  knob6 sequencer tempo
  knobs 1 to 6 sends MIDI CC on USB and DIN connectors

  CC BY-NC-SA
  pangrus 2022
*/

#include <MozziGuts.h>
#include <Oscil.h>
#include <LowPassFilter.h>
#include <mozzi_rand.h>
#include <mozzi_midi.h>
#include <ADSR.h>
#include <Metronome.h>
#include <MIDI.h>
#include <Adafruit_TinyUSB.h>
#include <tables/square_analogue512_int8.h>

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI_USB);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI_DIN);

// MIDI
bool isUsbStarted = LOW;
bool isDinStarted = LOW;
bool isMidiNote = LOW;
byte ppq24;
byte usbChannel = 1;
byte dinChannel = 1;

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
byte knobThreshold = 1;
byte selectedKnob;

// sequencer variables
float bpm;
byte lenght = 16;
int selectedPattern = 0;
int currentStep = 0;
int note;

// pushbuttons management variables
bool readPb1;
bool readPb2;
bool pb1State;
bool pb2State;
bool isStarted = LOW;
bool normalSequencing = HIGH;
bool lastPb1State = LOW;
bool lastPb2State = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 30000;

// sound generation variables
byte cutoffFrequency;
byte filterResonance;
byte detune;
int releaseTime;
long out;
float mainFrequency;

// selectedPatterns are made by midi notes (-1 for pause)
const int pattern[8][16] = {
  {36, 39, 41, 39, 46, 44, 46, 48, 36, 39, 41, 39, 46, 44, 46, 48},   // On The Run
  {41, 36, 51, 36, 48, 36, 36, 48, 36, 48, 36, 48, 46, 36, 48, 36},   // Der Mussolini (The Mussolini)
  {38, 37, 26, 38, 26, 26, 36, 26, 36, 37, 26, 38, 26, 26, 36, 26},   // Verschwende deine Jugend (Waste your youth)
  {55, 48, 48, 50, 50, 43, 50, 43, 55, 48, 48, 50, 50, 43, 50, 43},   // Mein Herz macht Bum (My heart goes boom)
  {49, 37, 37, 52, 49, 37, 37, 46, 49, 37, 37, 52, 49, 37, 37, 46},   // Muskel (Muscle)
  {52, 40, 41, 46, 40, 40, 50, 40, 52, 40, 41, 46, 40, 40, 50, 40},   // Verlieb dich in mich (Fall in love with me)
  {48, 48, 60, 60, 43, 43, 46, 46, 48, 48, 60, 60, 43, 43, 46, 46},   // I feel love
  {55, 40, 40, 54, 40, 40, 55, 40, 40, 54, 40, 40, 55, 40, 54, 40}    // Als wär's das letzte Mal (Like it was the last time)
};

Oscil<SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> oscillator1(SQUARE_ANALOGUE512_DATA);
Oscil<SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> oscillator2(SQUARE_ANALOGUE512_DATA);
Oscil<SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> oscillator3(SQUARE_ANALOGUE512_DATA);
Oscil<SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> oscillator4(SQUARE_ANALOGUE512_DATA);
ADSR <CONTROL_RATE, AUDIO_RATE> envelope;
LowPassFilter lowPass;

Metronome seqMetro(120);

void setup() {
  startMozzi();
  analogReadResolution (7);   // 127 is the maximum value from ADC
  analogWriteResolution (10);
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_LED3, OUTPUT);
  envelope.setTimes(0, 200000, 200000, 1000);
  SerialUSB.begin(115200);
  MIDI_USB.begin(usbChannel);
  MIDI_USB.setHandleStart(UsbHandleStart);
  MIDI_USB.setHandleStop(UsbHandleStop);
  MIDI_USB.setHandleNoteOn(UsbHandleNoteOn);
  MIDI_USB.setHandleNoteOff(UsbHandleNoteOff);
  MIDI_USB.setHandleClock(UsbHandleClock);
  MIDI_USB.setHandleControlChange(UsbHandleCC);
  MIDI_USB.turnThruOff();
  MIDI_DIN.begin(dinChannel);
  MIDI_DIN.setHandleStart(DinHandleStart);
  MIDI_DIN.setHandleStop(DinHandleStop);
  MIDI_DIN.setHandleNoteOn(DinHandleNoteOn);
  MIDI_DIN.setHandleNoteOff(DinHandleNoteOff);
  MIDI_DIN.setHandleClock(DinHandleClock);
  MIDI_DIN.setHandleControlChange(DinHandleCC);
  MIDI_DIN.turnThruOff();
  storedKnob[0] = analogRead(1);
  storedKnob[1] = analogRead(2);
  storedKnob[2] = analogRead(3);
  storedKnob[3] = analogRead(4);
  storedKnob[4] = analogRead(5);
  storedKnob[5] = analogRead(8);
}

void loop() {
  audioHook();
  MIDI_USB.read();
  MIDI_DIN.read();
}

void updateControl() {
  managePushbuttons();
  manageKnobs();
  cutoffFrequency = map (storedKnob[1], 0, 127, 25, 255);
  filterResonance = storedKnob[2] << 1;
  detune = storedKnob[2] >> 4;
  releaseTime = (storedKnob[0] * 3) + 30;
  if (!normalSequencing) {
    cutoffFrequency = rand (25, (char)cutoffFrequency);
    releaseTime = rand(releaseTime, releaseTime * 4);
  }
  envelope.setADLevels(255, 255);
  envelope.update();
  selectedPattern = storedKnob[3] / 16;
  bpm = 100 + (storedKnob[5] * 5);
  seqMetro.setBPM (bpm);
  if (seqMetro.ready() and isStarted) triggerEnvelope();
}

void triggerEnvelope() {
  if (!normalSequencing and rand(6) > 4) note = pattern[selectedPattern][currentStep] + map (storedKnob[4], 0, 127, 0, 12) + rand(2) * 12 - rand(2) * 12;
  else note = pattern[selectedPattern][currentStep] + map (storedKnob[4], 0, 127, 0, 12);
  mainFrequency = mtof(note);
  oscillator1.setFreq(mainFrequency);
  oscillator2.setFreq(mainFrequency / 2);
  oscillator3.setFreq(mainFrequency + detune);
  oscillator4.setFreq(mainFrequency - detune);
  lowPass.setCutoffFreqAndResonance(cutoffFrequency , filterResonance);
  envelope.setReleaseTime(releaseTime);
  if (note >= 0) {
    envelope.noteOn();
    envelope.noteOff();
  }

  if (normalSequencing) {
    currentStep++;
    if (currentStep > 15) currentStep = 0;
  }
  else {
    currentStep = currentStep + (9 - storedKnob[2] / 15);
    if (currentStep > 15) currentStep = 9 - storedKnob[2] / 15;
  }
}

AudioOutput_t updateAudio() {
  out = (envelope.next() * lowPass.next(
           oscillator1.next() +
           oscillator2.next() +
           oscillator3.next() +
           oscillator4.next())) >> 9;
  return out;
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
    }
  }
}

void managePushbuttons() {
  // PB1
  readPb1 = digitalRead(9);
  if (readPb1 != lastPb1State) {
    lastDebounceTime = mozziMicros();
  }
  if ((mozziMicros() - lastDebounceTime) > debounceDelay) {
    if (readPb1 != pb1State) {
      pb1State = readPb1;
      if (pb1State == LOW) {
        isStarted = !isStarted;
        if (isStarted) {
          currentStep = 0;
          ppq24 = 0;
          seqMetro.start();
          Serial.println ("Start");
        }
        else {
          envelope.noteOff();
          seqMetro.stop();
          currentStep = 0;
          Serial.println ("Stop");
        }
        digitalWrite (PIN_LED3, !isStarted);    //PIN_LED3 is lighted on when LOW
      }
    }
  }
  lastPb1State = readPb1;

  // PB2 toggles generative sequencing
  readPb2 = digitalRead(10);
  if (readPb2 != lastPb2State) {
    lastDebounceTime = mozziMicros();
  }
  if ((mozziMicros() - lastDebounceTime) > debounceDelay) {
    if (readPb2 != pb2State) {
      pb2State = readPb2;
      if (pb2State == LOW) {
        normalSequencing = !normalSequencing;
        digitalWrite (PIN_LED2, normalSequencing);
        currentStep = 0;
      }
    }
  }
  lastPb2State = readPb2;
}

void UsbHandleStart() {
  ppq24 = 0;
  currentStep = 0;
  isUsbStarted = HIGH;
  digitalWrite (PIN_LED3, LOW);
  SerialUSB.println("USB MIDI Start");
}

void UsbHandleStop() {
  ppq24 = 0;
  currentStep = 0;
  isUsbStarted = LOW;
  digitalWrite (PIN_LED3, HIGH);
  SerialUSB.println("USB MIDI Stop");
}

void UsbHandleClock() {
  if (isUsbStarted) {
    if (ppq24 == 0) triggerEnvelope();
    if (ppq24++ > 4) ppq24 = 0;
  }
}

void DinHandleStart() {
  ppq24 = 0;
  currentStep = 0;
  isDinStarted = HIGH;
  digitalWrite (PIN_LED3, LOW);
  SerialUSB.println("DIN MIDI Start");
}

void DinHandleStop() {
  ppq24 = 0;
  currentStep = 0;
  isDinStarted = LOW;
  digitalWrite (PIN_LED3, HIGH);
  SerialUSB.println("DIN --> USB MIDI Stop");
}

void DinHandleClock() {
  if (isDinStarted) {
    if (ppq24 == 0) triggerEnvelope();
    if (ppq24++ > 4) ppq24 = 0;
  }
}

void UsbHandleNoteOn(byte channel, byte note, byte velocity) {
  lowPass.setCutoffFreqAndResonance(cutoffFrequency , filterResonance);
  envelope.setReleaseTime(releaseTime);
  mainFrequency = mtof(note);
  oscillator1.setFreq(mainFrequency);
  oscillator2.setFreq(mainFrequency / 2);
  oscillator3.setFreq(mainFrequency + detune);
  oscillator4.setFreq(mainFrequency - detune);
  if (velocity > 0) envelope.noteOn();
  else envelope.noteOff();
}

void UsbHandleNoteOff(byte channel, byte note, byte velocity) {
  envelope.noteOff();
}

void UsbHandleCC(byte channel, byte control, byte value) {
}

void DinHandleNoteOn(byte channel, byte note, byte velocity) {
}

void DinHandleNoteOff(byte channel, byte note, byte velocity) {
}

void DinHandleCC(byte channel, byte control, byte value) {
}
