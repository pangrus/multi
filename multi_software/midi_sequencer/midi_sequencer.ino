/*
           _     _ _                                                 
 _ __ ___ (_) __| (_)  ___  ___  __ _ _   _  ___ _ __   ___ ___ _ __ 
| '_ ` _ \| |/ _` | | / __|/ _ \/ _` | | | |/ _ \ '_ \ / __/ _ \ '__|
| | | | | | | (_| | | \__ \  __/ (_| | |_| |  __/ | | | (_|  __/ |   
|_| |_| |_|_|\__,_|_| |___/\___|\__, |\__,_|\___|_| |_|\___\___|_|   
                                   |_|                               
  midi sequencer V1.0
  ---------------------
  
  CC BY-NC-SA
  pangrus 2025
*/

#include <Mozzi.h>
#include <Oscil.h>
#include <mozzi_rand.h>
#include <mozzi_midi.h>
#include <ADSR.h>
#include <Metronome.h>
#include <MIDI.h>
#include <Adafruit_TinyUSB.h>

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
bool blink = HIGH;
byte lenght = 16;
int selectedPattern = 0;
int currentStep = 0;
int note;
int octaveShift;
int sign;
float bpm;

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
  {36, 39, 41, 39, 46, 44, 46, 48, 36, 39, 41, 39, 46, 44, 46, 48},
  {51, 36, 48, 36, 36, 48, 36, 48, 36, 48, 46, 36, 48, 36, 41, 36},
  {38, 37, 26, 38, 26, 26, 36, 26, 36, 37, 26, 38, 26, 26, 36, 26},
  {55, 48, 48, 50, 50, 43, 50, 43, 55, 48, 48, 50, 50, 43, 50, 43},
  {49, 37, 37, 52, 49, 37, 37, 46, 49, 37, 37, 52, 49, 37, 37, 46},
  {52, 40, 41, 46, 40, 40, 50, 40, 52, 40, 41, 46, 40, 40, 50, 40},
  {48, 48, 60, 60, 43, 43, 46, 46, 48, 48, 60, 60, 43, 43, 46, 46},
  {55, 40, 40, 54, 40, 40, 55, 40, 40, 54, 40, 40, 55, 40, 54, 40}
};

Metronome seqMetro(120);

void setup() {
  startMozzi();
  analogReadResolution (7);   // 127 is the maximum value from ADC
  analogWriteResolution (10);
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_LED3, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  SerialUSB.begin(115200);
  MIDI_USB.begin(usbChannel);
  MIDI_USB.setHandleStart(UsbHandleStart);
  MIDI_USB.setHandleStop(UsbHandleStop);
  MIDI_USB.setHandleClock(UsbHandleClock);
  MIDI_USB.turnThruOn();
  MIDI_DIN.begin(dinChannel);
  MIDI_DIN.setHandleStart(DinHandleStart);
  MIDI_DIN.setHandleStop(DinHandleStop);
  MIDI_DIN.setHandleClock(DinHandleClock);
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
  if (seqMetro.ready() and isStarted) playMidi();
}

void updateControl() {
  managePushbuttons();
  manageKnobs();
  cutoffFrequency = map (storedKnob[1], 0, 127, 60, 255);
  filterResonance = map (storedKnob[2], 0, 127, 0, 230);
  detune = storedKnob[2] >> 4;
  releaseTime = (storedKnob[0] * 3) + 30;
  if (!normalSequencing) {
    cutoffFrequency = rand (25, (char)cutoffFrequency);
    releaseTime = rand(releaseTime, releaseTime * 4);
  }
  selectedPattern = storedKnob[3] / 16;
  bpm = 100 + (storedKnob[5] * 5);
  seqMetro.setBPM (bpm);
}

void playMidi() {
  note = pattern[selectedPattern][currentStep] + map (storedKnob[4], 0, 127, 0, 12);
  mainFrequency = mtof(note);

  if (!normalSequencing) {
      if (random(100) < 30) {                 // 30% chance
        int octaveShift = random(3) * 12;   // Random multiple of 12 (0, 12, 24)
        int sign = random(2) * 2 - 1;       // Randomly choose -1 or 1
        note += sign * octaveShift;
    }
  }
  
  if (note >= 0) {
    blink = !blink;
    digitalWrite(LED_BUILTIN, blink);
    if (normalSequencing) MIDI_DIN.sendNoteOn (note, 100, dinChannel);
    else MIDI_DIN.sendNoteOn (note, 100 + random(27), dinChannel);
    delay (2);
    MIDI_DIN.sendNoteOff (note, 0, dinChannel);
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
          seqMetro.stop();
          currentStep = 0;
          Serial.println ("Stop");
          blink = HIGH;
          digitalWrite(LED_BUILTIN, blink);
        }
        digitalWrite (PIN_LED3, !isStarted);
      }
    }
  }
  lastPb1State = readPb1;

  // PB2
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
    if (ppq24 == 0) playMidi();
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
    if (ppq24 == 0) playMidi();
    if (ppq24++ > 4) ppq24 = 0;
  }
}

AudioOutput_t updateAudio() {
}
