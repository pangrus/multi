/*
                                           _     _
    ___ ___   ___  ___ _ __ __ _ _ __ ___ | |__ | | ___ _ __
   / __/ __| / __|/ __| '__/ _` | '_ ` _ \| '_ \| |/ _ \ '__|
  | (_| (__  \__ \ (__| | | (_| | | | | | | |_) | |  __/ |
   \___\___| |___/\___|_|  \__,_|_| |_| |_|_.__/|_|\___|_|


  cc scrambler - preset fucker v0.3
  ---------------------------------
  PB1           scrambles MIDI CC from 0 to 119
  PB2 + KNOB6   change the MIDI channel (from 1 to 10)
  orange led blinks accordingly to the selected MIDI channel
  KNOB1 to KNOB6 sends the assigned MIDI CC

  CC BY-NC-SA pangrus 2022

  list of MIDI Control Change

  0   00  Bank Select
  1   01  Modulation Wheel or Lever
  2   02  Breath Controller
  3   03  Undefined
  4   04  Foot Controller
  5   05  Portamento Time
  6   06  Data Entry
  7   07  Channel Volume (formerly Main Volume)
  8   08  Balance
  9   09  Undefined
  10  0A  Pan
  11  0B  Expression Controller
  12  0C  Effect Control 1
  13  0D  Effect Control 2
  14  0E  Undefined
  15  0F  Undefined
  16  10  General Purpose Controller 1
  17  11  General Purpose Controller 2
  18  12  General Purpose Controller 3
  19  13  General Purpose Controller 4
  20  14  Undefined
  21  15  Undefined
  22  16  Undefined
  23  17  Undefined
  24  18  Undefined
  25  19  Undefined
  26  1A  Undefined
  27  1B  Undefined
  28  1C  Undefined
  29  1D  Undefined
  30  1E  Undefined
  31  1F  Undefined
  32  20  LSB for Control 0 (Bank Select)
  33  21  LSB for Control 1 (Modulation Wheel or Lever)
  34  22  LSB for Control 2 (Breath Controller)
  35  23  LSB for Control 3 (Undefined)
  36  24  LSB for Control 4 (Foot Controller)
  37  25  LSB for Control 5 (Portamento Time)
  38  26  LSB for Control 6 (Data Entry)
  39  27  LSB for Control 7 (Channel Volume, formerly Main Volume)
  40  28  LSB for Control 8 (Balance)
  41  29  LSB for Control 9 (Undefined)
  42  2A  LSB for Control 10 (Pan)
  43  2B  LSB for Control 11 (Expression Controller)
  44  2C  LSB for Control 12 (Effect control 1)
  45  2D  LSB for Control 13 (Effect control 2)
  46  2E  LSB for Control 14 (Undefined)
  47  2F  LSB for Control 15 (Undefined)
  48  30  LSB for Control 16 (General Purpose Controller 1)
  49  31  LSB for Control 17 (General Purpose Controller 2)
  50  32  LSB for Control 18 (General Purpose Controller 3)
  51  33  LSB for Control 19 (General Purpose Controller 4)
  52  34  LSB for Control 20 (Undefined)
  53  35  LSB for Control 21 (Undefined)
  54  36  LSB for Control 22 (Undefined)
  55  37  LSB for Control 23 (Undefined)
  56  38  LSB for Control 24 (Undefined)
  57  39  LSB for Control 25 (Undefined)
  58  3A  LSB for Control 26 (Undefined)
  59  3B  LSB for Control 27 (Undefined)
  60  3C  LSB for Control 28 (Undefined)
  61  3D  LSB for Control 29 (Undefined)
  62  3E  LSB for Control 30 (Undefined)
  63  3F  LSB for Control 31 (Undefined)
  64  40  Damper Pedal on/off (Sustain)   ≤63 off, ≥64 on
  65  41  Portamento On/Off   ≤63 off, ≥64 on
  66  42  Sostenuto On/Off  ≤63 off, ≥64 on
  67  43  Soft Pedal On/Off   ≤63 off, ≥64 on
  68  44  Legato Footswitch   ≤63 Normal, ≥64 Legato
  69  45  Hold 2  ≤63 off, ≥64 on
  70  46  Sound Controller 1 (default: Sound Variation)
  71  47  Sound Controller 2 (default: Timbre/Harmonic Intens.)
  72  48  Sound Controller 3 (default: Release Time)
  73  49  Sound Controller 4 (default: Attack Time)
  74  4A  Sound Controller 5 (default: Brightness)
  75  4B  Sound Controller 6 (default: Decay Time - see MMA RP-021)
  76  4C  Sound Controller 7 (default: Vibrato Rate - see MMA RP-021)
  77  4D  Sound Controller 8 (default: Vibrato Depth - see MMA RP-021)
  78  4E  Sound Controller 9 (default: Vibrato Delay - see MMA RP-021)
  79  4F  Sound Controller 10 (default undefined - see MMA RP-021)
  80  50  General Purpose Controller 5
  81  51  General Purpose Controller 6
  82  52  General Purpose Controller 7
  83  53  General Purpose Controller 8
  84  54  Portamento Control
  85  55  Undefined
  86  56  Undefined
  87  57  Undefined
  88  58  High Resolution Velocity Prefix
  89  59  Undefined
  90  5A  Undefined
  91  5B  Effects 1 Depth (Reverb Send Level)
  92  5C  Effects 2 Depth (formerly Tremolo Depth)
  93  5D  Effects 3 Depth (Chorus Send Level)
  94  5E  Effects 4 Depth (formerly Celeste [Detune] Depth)
  95  5F  Effects 5 Depth (formerly Phaser Depth)
  96  60  Data Increment (Data Entry +1) (see MMA RP-018)   N/A
  97  61  Data Decrement (Data Entry -1) (see MMA RP-018)   N/A
  98  62  Non-Registered Parameter Number (NRPN) -
  99  63  Non-Registered Parameter Number (NRPN) -
  100 64  Registered Parameter Number (RPN) -  *
  101 65  Registered Parameter Number (RPN) -  *
  102 66  Undefined
  103 67  Undefined
  104 68  Undefined
  105 69  Undefined
  106 6A  Undefined
  107 6B  Undefined
  108 6C  Undefined
  109 6D  Undefined
  110 6E  Undefined
  111 6F  Undefined
  112 70  Undefined
  113 71  Undefined
  114 72  Undefined
  115 73  Undefined
  116 74  Undefined
  117 75  Undefined
  118 76  Undefined
  119 77  Undefined
  120 78  [Channel Mode Message] All Sound Off
  121 79  [Channel Mode Message] Reset All Controllers
  122 7A  [Channel Mode Message] Local Control On/Off
  123 7B  [Channel Mode Message] All Notes Off
  124 7C  [Channel Mode Message] Omni Mode Off (+ all notes off)
  125 7D  [Channel Mode Message] Omni Mode On (+ all notes off)
  126 7E  [Channel Mode Message] Mono Mode On (+ poly off, + all notes off)
*/

#include <MIDI.h>
#include <Adafruit_TinyUSB.h>

// MIDI
Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI_USB);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI_DIN);
byte usbChannel = 1;
byte dinChannel = 1;

// assign here the MIDI CC to knobs
#define CC_KNOB1 73     // attack
#define CC_KNOB2 80     // decay
#define CC_KNOB3 64     // sustain
#define CC_KNOB4 72     // release
#define CC_KNOB5 74     // cutoff
#define CC_KNOB6 71     // resonance

// CC values
byte controlChangeValue[127];

// active midiChannel;
byte midiChannel = 1;

// ccscrambler blink variables
bool ledState;
byte blinks;
unsigned long currentMillis = 0;
unsigned long previousMillis = 0;
unsigned long blinkTime = 200;

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
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 20000;

void setup() {
  analogReadResolution (7);
  analogWriteResolution (10);
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_LED3, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  SerialUSB.begin(115200);
  MIDI_USB.begin(usbChannel);
  MIDI_DIN.begin(dinChannel);
  MIDI_USB.turnThruOff();
  MIDI_DIN.turnThruOff();
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
  managePushbuttons();
  manageKnobs();
  manageBlinks();
  if (pb1Mode) presetFucker();
  if (pb2Mode) midiChannel = map(storedKnob[5], 0, 120, 1, 10);
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
  digitalWrite (PIN_LED2, !pb1Mode);
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
          if (pb2Mode) {
            SerialUSB.print("Selected MIDI channel for CC scrambling: ");
            SerialUSB.println(midiChannel);
          }
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
    lastDebounceTime = micros();
  }
  if ((micros() - lastDebounceTime) > debounceDelay) {
    if (readPb1 != pb1State) {
      pb1State = readPb1;
      if (pb1State == LOW) {
        pb1Mode = !pb1Mode;
        digitalWrite (PIN_LED2, !pb1Mode);
      }
    }
  }
  lastPb1State = readPb1;
  // pb2
  readPb2 = digitalRead(10);
  if (readPb2 != lastPb2State) {
    lastDebounceTime = micros();
  }
  if ((micros() - lastDebounceTime) > debounceDelay) {
    if (readPb2 != pb2State) {
      pb2State = readPb2;
      if (pb2State == LOW) {
        pb2Mode = !pb2Mode;
        digitalWrite (PIN_LED3, !pb2Mode);
      }
    }
  }
  lastPb2State = readPb2;
}
