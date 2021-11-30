/*
                   _ _   _
                  | | | (_)
   _ __ ___  _   _| | |_ _
  | '_ ` _ \| | | | | __| |
  | | | | | | |_| | | |_| |
  |_| |_| |_|\__,_|_|\__|_|

  midi converter 
  USB MIDI <---> MIDI DIN

  pangrus 2021
*/





#include <Adafruit_TinyUSB.h>
#include <MIDI.h>

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI_USB);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI_DIN);

void setup() {
  SerialUSB.begin(115200);
  MIDI_USB.begin(MIDI_CHANNEL_OMNI);
  MIDI_USB.setHandleStart(UsbHandleStart);
  MIDI_USB.setHandleStop(UsbHandleStop);
  MIDI_USB.setHandleNoteOn(UsbHandleNoteOn);
  MIDI_USB.setHandleNoteOff(UsbHandleNoteOff);
  MIDI_USB.setHandleClock(UsbHandleClock);
  MIDI_USB.setHandleControlChange(UsbHandleCC);
  MIDI_DIN.begin(MIDI_CHANNEL_OMNI);
  MIDI_DIN.setHandleStart(DinHandleStart);
  MIDI_DIN.setHandleStop(DinHandleStop);
  MIDI_DIN.setHandleNoteOn(DinHandleNoteOn);
  MIDI_DIN.setHandleNoteOff(DinHandleNoteOff);
  MIDI_DIN.setHandleClock(DinHandleClock);
  MIDI_DIN.setHandleControlChange(DinHandleCC);
  MIDI_DIN.turnThruOff();
}

void loop() {
  MIDI_USB.read();
  MIDI_DIN.read();
}

void UsbHandleStart() {
  MIDI_DIN.sendStart();
  SerialUSB.println("USB --> DIN MIDI Start");
}

void UsbHandleStop() {
  MIDI_DIN.sendStop();
  SerialUSB.println("USB --> DIN MIDI Stop");
}

void UsbHandleClock() {
  MIDI_DIN.sendClock();
}

void UsbHandleNoteOn(byte channel, byte note, byte velocity) {
  MIDI_DIN.sendNoteOn(note, velocity, channel);
  SerialUSB.print("USB --> DIN Note on: ");
  SerialUSB.print(note);
  SerialUSB.print(" Channel: ");
  SerialUSB.print(channel);
  SerialUSB.print(" Velocity: ");
  SerialUSB.println(velocity);
}

void UsbHandleNoteOff(byte channel, byte note, byte velocity) {
  MIDI_DIN.sendNoteOff(note, velocity, channel);
}

void UsbHandleCC(byte channel, byte control, byte value) {
  MIDI_DIN.sendControlChange(control, value, channel);
  SerialUSB.print("USB --> DIN Control Change: ");
  SerialUSB.print(control);
  SerialUSB.print(" Channel: ");
  SerialUSB.print(channel);
  SerialUSB.print(" Value: ");
  SerialUSB.println(value);
}

void DinHandleStart() {
  MIDI_USB.sendStart();
  SerialUSB.println("DIN --> USB MIDI Start");
}

void DinHandleStop() {
  MIDI_USB.sendStop();
  SerialUSB.println("DIN --> USB MIDI Stop");
}

void DinHandleClock() {
  MIDI_USB.sendClock();
}

void DinHandleNoteOn(byte channel, byte note, byte velocity) {
  MIDI_USB.sendNoteOn(note, velocity, channel);
  SerialUSB.print("DIN --> USB Note on: ");
  SerialUSB.print(note);
  SerialUSB.print(" Channel: ");
  SerialUSB.print(channel);
  SerialUSB.print(" Velocity: ");
  SerialUSB.println(velocity);
}

void DinHandleNoteOff(byte channel, byte note, byte velocity) {
  MIDI_USB.sendNoteOff(note, velocity, channel);
  SerialUSB.print("DIN --> USB Note off: ");
  SerialUSB.print(note);
  SerialUSB.print(" Channel: ");
  SerialUSB.print(channel);
  SerialUSB.print(" Velocity: ");
  SerialUSB.println(velocity);
}

void DinHandleCC(byte channel, byte control, byte value) {
  MIDI_USB.sendControlChange(control, value, channel);
  SerialUSB.print("DIN --> USB Control Change: ");
  SerialUSB.print(control);
  SerialUSB.print(" Channel: ");
  SerialUSB.print(channel);
  SerialUSB.print(" Value: ");
  SerialUSB.println(value);
}
