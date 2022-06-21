/*
                   _ _   _                     _
   _ __ ___  _   _| | |_(_)_ __ ___   ___   __| | ___
  | '_ ` _ \| | | | | __| | '_ ` _ \ / _ \ / _` |/ _ \
  | | | | | | |_| | | |_| | | | | | | (_) | (_| |  __/
  |_| |_| |_|\__,_|_|\__|_|_| |_| |_|\___/ \__,_|\___|

  multimode v2.0
  CC BY-NC-SA pangrus 2022
  ------------------------

  knobs 1 to 6 are used to select the mode at power up
  knobs 1 to 6 are sending MIDI CC on USB and DIN connectors

  select mode:
  ------------
  set all the knobs fully CCW
  select mode by setting only one knob fully CW (KNOB1 -> mode 1...)
  keep pressed PB1 and PB2 for 4 seconds

  mode 1 : synth sequencer
  ------------------------
  knob1         synth release
  knob2         synth cutoff
  knob3         synth resonance and detune, step advance amount
  knob4         selects sequence
  knob5         transpose sequence
  knob6         sequencer tempo

  mode 2 : drone
  6 oscillators drone generator with 6 lfos
  -----------------------------------------
  knobs 1 to 6  controls the oscillator frequency and midi controller
  pb1 switches  drone on/off
  pb2 switches  lfos on/off

  mode 3 : euclidean drum sequencer
  ----------------------------
  receives midi clock, start/stop on the MIDI DIN input
  generates euclidean rhythmes on the MIDI DIN output
  knob 1,2,3 selects number of beats to be played
  knob 4,5,6 pattern length
  pb1 randomize pattern 2
  pb2 randomize pattern 3

  mode 4: bytebeat player
  -----------------------
  knobs are affecting expression parameters
  pb1 and pb2 selects different expressions

  mode 5: fourier (3 voices drone generator)
  ------------------------------------------
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

  mode 6: cc scrambler
  ------------------------------------------
  PB1           scrambles MIDI CC from 0 to 119
  PB2 + KNOB6   change the MIDI channel (from 1 to 10)
  orange led blinks accordingly to the selected MIDI channel
  KNOB1 to KNOB6 sends the assigned MIDI CC
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

// wavetables
#include <tables/triangle2048_int8.h>
#include <tables/square_analogue512_int8.h>
#include <tables/sin4096_int8.h>

// mode
byte modeSelect = 1;  //synth mode is the default mode

// MIDI
Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI_USB);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI_DIN);

// MIDI variables
bool isUsbStarted = LOW;
bool isDinStarted = LOW;
bool isMidiNote = LOW;
byte ppq24;
byte usbChannel = 1;
byte dinChannel = 10;

// assign MIDI CC to knobs
#define CC_KNOB1 73     // attack
#define CC_KNOB2 80     // decay
#define CC_KNOB3 64     // sustain
#define CC_KNOB4 72     // release
#define CC_KNOB5 74     // cutoff
#define CC_KNOB6 71     // resonance

// MIDI notes for euclidean sequencer
byte midiNote0 = 60;  // C3
byte midiNote1 = 62;  // D3
byte midiNote2 = 64;  // E3

// knobs are debounced and values are stored
byte actualKnob[] = {0, 0, 0, 0, 0, 0, 0};
byte storedKnob[] = {0, 0, 0, 0, 0, 0, 0};
byte knobThreshold = 3;
byte selectedKnob;

// synth sequencer variables
float bpm;
byte lenght = 16;
int selectedPattern = 0;
int currentStep = 0;
int note;

// euclidean sequencer variables
char euclideanPattern[3][16];
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
byte cutoffFrequency;
byte filterResonance;
byte detune;
int releaseTime;
long synthOut;
float mainFrequency;

// drone variables
byte midiNote[6];
byte amplitude1, amplitude2, amplitude3, amplitude4, amplitude5, amplitude6;
int freq[6];
float lfoFreq1, lfoFreq2, lfoFreq3, lfoFreq4, lfoFreq5, lfoFreq6;
long droneOut;

// bytebeat variables
byte parameter1;
byte parameter2;
byte parameter3;
byte parameter4;
byte parameter5;
byte parameter6;
long t = 0;
long bytebeatOut = 0;
bool even;

// fourier variables
byte level[3][6];
int fourierFreq[3];
long fourierOut;

// cc scrambler variables
bool ledState;
byte blinks;
unsigned long currentMillis = 0;
unsigned long previousMillis = 0;
unsigned long blinkTime = 200;
byte midiChannel = 1;
byte controlChangeValue[127];

// reset
const int RESET_TIME  = 3000000;
int lastState = LOW;
int resetState;
unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;
bool isPressing = LOW;
bool isLongDetected = LOW;
long pressDuration;


// patterns are made by midi notes (-1 for pause)
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

// drone oscillators
Oscil <TRIANGLE2048_NUM_CELLS, AUDIO_RATE> droneOsc1(TRIANGLE2048_DATA);
Oscil <TRIANGLE2048_NUM_CELLS, AUDIO_RATE> droneOsc2(TRIANGLE2048_DATA);
Oscil <TRIANGLE2048_NUM_CELLS, AUDIO_RATE> droneOsc3(TRIANGLE2048_DATA);
Oscil <TRIANGLE2048_NUM_CELLS, AUDIO_RATE> droneOsc4(TRIANGLE2048_DATA);
Oscil <TRIANGLE2048_NUM_CELLS, AUDIO_RATE> droneOsc5(TRIANGLE2048_DATA);
Oscil <TRIANGLE2048_NUM_CELLS, AUDIO_RATE> droneOsc6(TRIANGLE2048_DATA);

// 6 lfos
Oscil<TRIANGLE2048_NUM_CELLS, CONTROL_RATE> lfo1(TRIANGLE2048_DATA);
Oscil<TRIANGLE2048_NUM_CELLS, CONTROL_RATE> lfo2(TRIANGLE2048_DATA);
Oscil<TRIANGLE2048_NUM_CELLS, CONTROL_RATE> lfo3(TRIANGLE2048_DATA);
Oscil<TRIANGLE2048_NUM_CELLS, CONTROL_RATE> lfo4(TRIANGLE2048_DATA);
Oscil<TRIANGLE2048_NUM_CELLS, CONTROL_RATE> lfo5(TRIANGLE2048_DATA);
Oscil<TRIANGLE2048_NUM_CELLS, CONTROL_RATE> lfo6(TRIANGLE2048_DATA);

// 4 oscillators synth
Oscil<SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> oscillator1(SQUARE_ANALOGUE512_DATA);
Oscil<SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> oscillator2(SQUARE_ANALOGUE512_DATA);
Oscil<SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> oscillator3(SQUARE_ANALOGUE512_DATA);
Oscil<SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> oscillator4(SQUARE_ANALOGUE512_DATA);
ADSR <CONTROL_RATE, AUDIO_RATE> envelope;
LowPassFilter lowPass;

// fourier oscillators and lfos
Oscil < SIN4096_NUM_CELLS, AUDIO_RATE > fourierOsc[3][6];
Oscil < SIN4096_NUM_CELLS, CONTROL_RATE > fourierLfo[3][6];

Metronome seqMetro(120);

void setup() {
  startMozzi();
  analogReadResolution (7);
  analogWriteResolution (10);
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_LED3, OUTPUT);
  envelope.setTimes(0, 200000, 200000, 1000);
  SerialUSB.begin(115200);
  MIDI_USB.begin(usbChannel);
  MIDI_USB.setHandleStart(usbStart);
  MIDI_USB.setHandleStop(usbStop);
  MIDI_USB.setHandleNoteOn(usbNoteOn);
  MIDI_USB.setHandleNoteOff(usbNoteOff);
  MIDI_USB.setHandleClock(usbClock);
  MIDI_USB.setHandleControlChange(usbHandleCC);
  MIDI_USB.turnThruOff();
  MIDI_DIN.begin(dinChannel);
  MIDI_DIN.setHandleStart(dinStart);
  MIDI_DIN.setHandleStop(dinStop);
  MIDI_DIN.setHandleNoteOn(dinHandleNoteOn);
  MIDI_DIN.setHandleNoteOff(dinHandleNoteOff);
  MIDI_DIN.setHandleClock(dinClock);
  MIDI_DIN.setHandleControlChange(dinHandleCC);
  MIDI_DIN.turnThruOff();
  storedKnob[0] = analogRead(1);
  storedKnob[1] = analogRead(2);
  storedKnob[2] = analogRead(3);
  storedKnob[3] = analogRead(4);
  storedKnob[4] = analogRead(5);
  storedKnob[5] = analogRead(8);
  // lfo frequencies
  lfo1.setFreq(0.0131f);
  lfo2.setFreq(0.0181f);
  lfo3.setFreq(0.0239f);
  lfo4.setFreq(0.0421f);
  lfo5.setFreq(0.0557f);
  lfo6.setFreq(0.0673f);
  // init pattern used for pattern rotation
  for (int i = 0; i < 15; i++) {
    patternSwap[i] = '.';
  }
  for (int j = 0; j < 3; j++) {
    for (int i = 0; i < 6; i++) {
      fourierOsc[j][i].setTable(SIN4096_DATA);
      fourierLfo[j][i].setTable(SIN4096_DATA);
    }
  }
  // mode selection
  if (analogRead(1) > 110) modeSelect = 1;
  if (analogRead(2) > 110) modeSelect = 2;
  if (analogRead(3) > 110) modeSelect = 3;
  if (analogRead(4) > 110) modeSelect = 4;
  if (analogRead(5) > 110) modeSelect = 5;
  if (analogRead(8) > 110) modeSelect = 6;
}

void loop() {
  audioHook();
  MIDI_USB.read();
  MIDI_DIN.read();
}

void updateControl() {
  managePushbuttons();
  manageKnobs();
  switch (modeSelect) {
    case 1:
      synth();
      break;
    case 2:
      drone();
      break;
    case 3:
      euclide();
      break;
    case 4:
      bytebeat();
      break;
    case 5:
      fourier();
      break;
    case 6:
      manageBlinks();
      if (pb1Mode) presetFucker();
      if (pb2Mode) midiChannel = map(storedKnob[5], 0, 120, 1, 10);
      break;
  }
}

// synth
void synth() {
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
  if (seqMetro.ready() and isStarted) playSynthNote();
}

void playSynthNote() {
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

// drone
void drone() {
  if (pb2Mode) {
    amplitude1 = map (lfo1.next(), -127, 127, 10, 240);
    amplitude2 = map (lfo2.next(), -127, 127, 10, 240);
    amplitude3 = map (lfo3.next(), -127, 127, 10, 240);
    amplitude4 = map (lfo4.next(), -127, 127, 10, 240);
    amplitude5 = map (lfo5.next(), -127, 127, 10, 240);
    amplitude6 = map (lfo6.next(), -127, 127, 10, 240);
  }
  else {
    amplitude1 = amplitude2 = amplitude3 = amplitude4 = amplitude5 = amplitude6 = 200;
  }
  for (int i = 0; i < 6; i++) {
    midiNote[i] = map (storedKnob[i], 0, 120, 36, 72);
    freq[i] = mtof(midiNote[i]);
  }
  droneOsc1.setFreq(freq[0]);
  droneOsc2.setFreq(freq[1]);
  droneOsc3.setFreq(freq[2]);
  droneOsc4.setFreq(freq[3]);
  droneOsc5.setFreq(freq[4]);
  droneOsc6.setFreq(freq[5]);
}

// euclidean drum sequencer
void euclide() {
  beats1 = map (storedKnob[0], 0, 120, 0, 16);
  beats2 = map (storedKnob[1], 0, 120, 0, 16);
  beats3 = map (storedKnob[2], 0, 120, 0, 16);
  length1 = map (storedKnob[3], 0, 120, 1, 16);
  length2 = map (storedKnob[4], 0, 120, 1, 16);
  length3 = map (storedKnob[5], 0, 120, 1, 16);
  createPattern(0, beats1, length1);
  createPattern(1, beats2, length2);
  createPattern(2, beats3, length3);
  if (pb1Mode) createPattern(1, rand (beats2), 16);
  if (pb2Mode) createPattern(2, rand (beats3), 16);
  rotatePattern(1, 4);
  rotatePattern(2, 2);
}

// bytebeat
void bytebeat() {
  parameter1 = storedKnob[0] >> 3;
  parameter2 = storedKnob[1] >> 3;
  parameter3 = storedKnob[2] >> 3;
  parameter4 = storedKnob[3] >> 3;
  parameter5 = storedKnob[4] >> 3;
  parameter6 = storedKnob[5] >> 3;
}

// fourier
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

void manageBlinks() {
  currentMillis = millis();
  if (currentMillis - previousMillis >= blinkTime) {
    previousMillis = currentMillis;
    if (ledState == LOW) ledState = HIGH;
    else {
      ledState = LOW;
      blinks++;
      if (blinks > midiChannel) {
        blinks = 0;
        ledState = HIGH;
      }
    }
    digitalWrite(LED_BUILTIN, ledState);
  }
}

void presetFucker() {
  for (int i = 0; i < 120; i++) {
    controlChangeValue[i] = random(128);
    MIDI_USB.sendControlChange(i, controlChangeValue[i], midiChannel);
    MIDI_DIN.sendControlChange(i, controlChangeValue[i], midiChannel);
    delay (5);
  }
  SerialUSB.print("Scrambled CC on MIDI channel: ");
  SerialUSB.println(midiChannel);
  pb1Mode = 0;
  digitalWrite (PIN_LED3, !pb1Mode);
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
        isStarted = !isStarted;
        pb1Mode = !pb1Mode;
        if (modeSelect != 6) {
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
        }
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
        normalSequencing = !normalSequencing;
        digitalWrite (PIN_LED2, !pb2Mode);
        currentStep = 0;
      }
    }
  }
  lastPb2State = readPb2;

  // select mode by keeping pressed PB1 and PB2
  if (!readPb1 and !readPb2) resetState = HIGH;
  else resetState = LOW;
  if ( lastState == LOW && resetState == HIGH ) {
    pressedTime = mozziMicros();
    isPressing = HIGH;
    isLongDetected = LOW;
  } else if (lastState == HIGH && resetState == LOW) {
    isPressing = LOW;
  }
  if ( isPressing == HIGH && isLongDetected == LOW) {
    pressDuration = mozziMicros() - pressedTime;
    if ( pressDuration > RESET_TIME ) {
      Serial.println("Reset");
      if (analogRead(1) > 110) modeSelect = 1;
      if (analogRead(2) > 110) modeSelect = 2;
      if (analogRead(3) > 110) modeSelect = 3;
      if (analogRead(4) > 110) modeSelect = 4;
      if (analogRead(5) > 110) modeSelect = 5;
      if (analogRead(8) > 110) modeSelect = 6;
      pb1Mode = LOW;
      pb2Mode = LOW;
      lastPb1State = LOW;
      lastPb2State = LOW;
      isLongDetected = HIGH;
      digitalWrite (PIN_LED2, HIGH);
      digitalWrite (PIN_LED3, HIGH);
      isUsbStarted = LOW;
      isDinStarted = LOW;
      isMidiNote = LOW;
      ppq24 = 0;
      usbChannel = 1;
      dinChannel = 10;
      normalSequencing = HIGH;
      isStarted = LOW;
    }
  }
  lastState = resetState;
}

// MIDI
void usbStart() {
  if (modeSelect == 1) {
    ppq24 = 0;
    clockCounter = 0;
    currentStep = 0;
    activeStep1 = activeStep2 = activeStep3 = 0;
    isUsbStarted = HIGH;
    SerialUSB.println("USB MIDI Start");
  }
}

void usbStop() {
  if (modeSelect == 1) {
    ppq24 = 0;
    clockCounter == 0;
    currentStep = 0;
    activeStep1 = activeStep2 = activeStep3 = 0;
    isUsbStarted = LOW;
    SerialUSB.println("USB MIDI Stop");
    digitalWrite (PIN_LED3, HIGH);
  }
}

void dinStart() {
  if (modeSelect == 1 or modeSelect == 3) {
    ppq24 = 0;
    clockCounter = 0;
    currentStep = 0;
    activeStep1 = activeStep2 = activeStep3 = 0;
    isDinStarted = HIGH;
    SerialUSB.println("DIN MIDI Start");
  }
}

void dinStop() {
  if (modeSelect == 1 or modeSelect == 3) {
    ppq24 = 0;
    clockCounter = 0;
    currentStep = 0;
    activeStep1 = activeStep2 = activeStep3 = 0;
    isDinStarted = LOW;
    SerialUSB.println("DIN MIDI Stop");
    digitalWrite (LED_BUILTIN, HIGH);
  }
}

void dinClock() {
  if (isDinStarted and modeSelect == 1) {
    if (ppq24 == 0) playSynthNote();
    if (ppq24++ > 4) ppq24 = 0;
    digitalWrite (LED_BUILTIN, ppq24);
  }
  if (isDinStarted and modeSelect == 3) {
    if (clockCounter == 0) {
      if (euclideanPattern[0][activeStep1] == 'x') MIDI_DIN.sendNoteOn (midiNote0, 127, dinChannel);
      if (euclideanPattern[1][activeStep2] == 'x') MIDI_DIN.sendNoteOn (midiNote1, 127, dinChannel);
      if (euclideanPattern[2][activeStep3] == 'x') MIDI_DIN.sendNoteOn (midiNote2, 127, dinChannel);
      activeStep1++;
      activeStep2++;
      activeStep3++;
      if (activeStep1 > length1 - 1) activeStep1 = 0;
      if (activeStep2 > length2 - 1) activeStep2 = 0;
      if (activeStep3 > length3 - 1) activeStep3 = 0;
      MIDI_DIN.sendNoteOff (midiNote0, 0, dinChannel);
      MIDI_DIN.sendNoteOff (midiNote1, 0, dinChannel);
      MIDI_DIN.sendNoteOff (midiNote2, 0, dinChannel);
    }
    if (clockCounter++ > 4) clockCounter = 0;
    digitalWrite (LED_BUILTIN, clockCounter);
  }
}

void usbClock() {
  if (isUsbStarted) {
    if (ppq24 == 0) playSynthNote();
    if (ppq24++ > 4) ppq24 = 0;
    digitalWrite (LED_BUILTIN, ppq24);
  }
}

void usbNoteOn(byte channel, byte note, byte velocity) {
  mainFrequency = mtof(note);
  oscillator1.setFreq(mainFrequency);
  oscillator2.setFreq(mainFrequency / 2);
  oscillator3.setFreq(mainFrequency + detune);
  oscillator4.setFreq(mainFrequency - detune);
  if (velocity > 0) envelope.noteOn();
  else envelope.noteOff();
}

void usbNoteOff(byte channel, byte note, byte velocity) {
  envelope.noteOff();
}

void usbHandleCC(byte channel, byte control, byte value) {
}

void dinHandleNoteOn(byte channel, byte note, byte velocity) {
}

void dinHandleNoteOff(byte channel, byte note, byte velocity) {
}

void dinHandleCC(byte channel, byte control, byte value) {
}

void createPattern(byte patternNumber, byte colpi, byte lengthPattern) {
  int cur = lengthPattern;
  for (int i = 0; i < lengthPattern; i++) {
    euclideanPattern[patternNumber][i] = '.';
    if (cur >= lengthPattern) {
      cur -= lengthPattern;
      euclideanPattern[patternNumber][i] = 'x';
    }
    if (colpi == 0) euclideanPattern[patternNumber][i] = '.';
    cur += colpi;
  }
}

void rotatePattern(byte patternNumber, byte rotateAmount) {
  for (int i = 0; i < 16; i++) {
    patternSwap[i] = euclideanPattern[patternNumber][i];
  }
  for (int i = 0; i < 16; i++) {
    if  (i < rotateAmount) euclideanPattern[patternNumber][i] = patternSwap[i + 16 - rotateAmount];
    else euclideanPattern[patternNumber][i] = patternSwap[i - rotateAmount];
  }
}

AudioOutput_t updateAudio() {
  // no audio during reset
  if (!digitalRead (8) and !digitalRead(9)) {
    return 0;
  }
  switch (modeSelect) {
    case 1:
      synthOut = (
                   envelope.next() * lowPass.next(
                     oscillator1.next() +
                     oscillator2.next() +
                     oscillator3.next() +
                     oscillator4.next())) >> 9;
      return synthOut;
      break;
    case 2:
      if (pb1Mode) {
        droneOut = (
                     droneOsc1.next() * amplitude1 +
                     droneOsc2.next() * amplitude2 +
                     droneOsc3.next() * amplitude3 +
                     droneOsc4.next() * amplitude4 +
                     droneOsc5.next() * amplitude5 +
                     droneOsc6.next() * amplitude6 ) >> 9;
      }
      else {
        droneOut = 0;
      }
      return droneOut;
      break;
    case 4:
      bytebeatOut = t * (parameter1 + (1 ^ t >> parameter2 & parameter3)) * (5 + (parameter4 & t >> parameter5)) >> (t >> 8 & parameter6);
      if (pb2Mode) bytebeatOut = t * (parameter6 + (1 ^ t >> parameter5 & parameter4)) * (5 + (parameter3 & t >> parameter2)) >> (t >> 8 & parameter1);
      if (pb1Mode & pb2Mode) bytebeatOut = (t * (parameter1 + (1 ^ t >> parameter2 & parameter3)) * (5 + (parameter4 & t >> parameter5)) >> (t >> 8 & parameter6)) ^ (t * (parameter6 + (1 ^ t >> parameter5 & parameter4)) * (5 + (parameter3 & t >> parameter2)) >> (t >> 8 & parameter1));
      if (!pb2Mode & !pb1Mode) {
        t = 0;
        bytebeatOut = 0;
      }
      t++;
      bytebeatOut = bytebeatOut >> 9;
      return bytebeatOut;
      break;
    case 5:
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
      break;
  }
}
