/*
                   _ _   _
                  | | | (_)
   _ __ ___  _   _| | |_ _
  | '_ ` _ \| | | | | __| |
  | | | | | | |_| | | |_| |
  |_| |_| |_|\__,_|_|\__|_|
  
  bytebeat player
  
  CC BY-NC-SA
  pangrus 2021  
*/

#include <TimerTC3.h>
#define USE_TIMER_TC3   // use TimerTc3
#define TIMER_TIME 125  // 1/8000 Hz = 125 micro seconds
#define TIMER TimerTc3
#define DAC_PIN 0       // pin of the XIAO DAC

byte parameter1;
byte parameter2;
byte parameter3;
byte parameter4;
byte parameter5;
byte parameter6;
long t = 0;
long out = 0;

void setup () {
  TIMER.initialize(TIMER_TIME);
  TIMER.attachInterrupt(GenerateAudioStream);
  pinMode (DAC_PIN, OUTPUT);
  analogReadResolution(5);
  analogWriteResolution(8);
  Serial.begin(9600);
}

void loop () {
  parameter1 = analogRead(1);
  parameter2 = analogRead(2);
  parameter3 = analogRead(3);
  parameter4 = analogRead(4);
  parameter5 = analogRead(5);
  parameter6 = analogRead(8);
}

void GenerateAudioStream() {
  // here's the bytebeat expresssion
  out = t * (parameter1 + (1 ^ t >> parameter2 & parameter3)) * (5 + (parameter4 & t >> parameter5)) >> (t >> 8 & parameter6);
  analogWrite (DAC_PIN, (byte)out);
  t++;
}
