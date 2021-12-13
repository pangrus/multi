/*
                   _ _   _
                  | | | (_)
   _ __ ___  _   _| | |_ _
  | '_ ` _ \| | | | | __| |
  | | | | | | |_| | | |_| |
  |_| |_| |_|\__,_|_|\__|_|
  
  MIDI metronome
  
  CC BY-NC-SA
  pangrus 2021
*/  

#include <MozziGuts.h>
#include <Sample.h>
#include <MIDI.h>

// wavetable data for the bamboo samples
#include <samples/bamboo/bamboo_01_2048_int8.h>
#include <samples/bamboo/bamboo_02_2048_int8.h> 

// Sample <table_size, update_rate> SampleName (wavetable)
Sample <BAMBOO_01_2048_NUM_CELLS, AUDIO_RATE>aBamboo1(BAMBOO_01_2048_DATA);
Sample <BAMBOO_02_2048_NUM_CELLS, AUDIO_RATE>aBamboo2(BAMBOO_02_2048_DATA);

// MIDI definition
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI_DIN);

// variables
bool isStarted;
int out;
int midiClock;

// pushbutton management
byte pb1Pin = 9;
byte pb2Pin = 10;
bool pb1State;
bool pb2State;
bool pb1Mode = HIGH;
bool pb2Mode = HIGH;
bool lastPb1State = LOW;
bool lastPb2State = LOW;
long lastDebounceTime = 0;
long debounceDelay = 50;

void setup() {
  startMozzi();
  aBamboo1.setFreq((float) BAMBOO_01_2048_SAMPLERATE / (float) BAMBOO_01_2048_NUM_CELLS); // play at the speed it was recorded at
  aBamboo2.setFreq((float) BAMBOO_02_2048_SAMPLERATE / (float) BAMBOO_02_2048_NUM_CELLS);
  SerialUSB.begin(115200);
  MIDI_DIN.begin(MIDI_CHANNEL_OMNI);
  MIDI_DIN.setHandleStart(DinHandleStart);
  MIDI_DIN.setHandleStop(DinHandleStop);
  MIDI_DIN.setHandleClock(DinHandleClock);
  MIDI_DIN.turnThruOff();
  analogReadResolution(4);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_LED3, OUTPUT);
  pinMode(pb1Pin, INPUT_PULLUP);
  pinMode(pb2Pin, INPUT_PULLUP);
}

void updateControl() {
  aBamboo1.setFreq((float)(analogRead(1) + 15));
  aBamboo2.setFreq((float)(analogRead(2) + 10));
}

void DinHandleStart() {
  SerialUSB.println("MIDI Start");
  aBamboo1.start();
  midiClock = 0;
  isStarted = true;
  digitalWrite (LED_BUILTIN, LOW);
}

void DinHandleStop() {
  SerialUSB.println("MIDI Stop");
  isStarted = false;
}

void DinHandleClock() {
  if (isStarted or !pb2Mode) {
    midiClock++;
    // 4/4
    if (pb1Mode) {
      if (midiClock == 24 or midiClock == 48 or midiClock == 72) {
        aBamboo2.start();
        digitalWrite (LED_BUILTIN, LOW);
      }
    }
    // 3/4
    else {
      if (midiClock == 32 or midiClock == 64) {
        aBamboo2.start();
        digitalWrite (LED_BUILTIN, LOW);
      }
    }
    if (midiClock == 96) {
      midiClock = 0;
      aBamboo1.start();
      digitalWrite (LED_BUILTIN, LOW);
    }
    delay(5);
    digitalWrite (LED_BUILTIN, HIGH);
  }
}

int updateAudio() {
  out = aBamboo1.next() + aBamboo2.next();
  return out;
}

void loop() {
  MIDI_DIN.read();
  audioHook();

  // pb1 selects 4/4 vs 3/4
  bool readPb1 = digitalRead(pb1Pin);
  if (readPb1 != lastPb1State) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (readPb1 != pb1State) {
      pb1State = readPb1;
      if (pb1State == LOW) {
        pb1Mode = !pb1Mode;
        digitalWrite (PIN_LED3, pb1Mode);
      }
    }
  }
  lastPb1State = readPb1;

  // press pb2 to ignore MIDI start/stop, metronome is linked directly to MIDI clock
  bool readPb2 = digitalRead(pb2Pin);
  if (readPb2 != lastPb2State) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (readPb2 != pb2State) {
      pb2State = readPb2;
      if (pb2State == LOW) {
        pb2Mode = !pb2Mode;
        digitalWrite (PIN_LED2, pb2Mode);
      }
    }
  }
  lastPb2State = readPb2;
}
