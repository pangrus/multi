## multi

![multi](multi_img/multi1_600.jpg)

**What is multi?**  
**multi** is an open hardware circuit board hosting a powerful [Seeeduino XIAO](https://wiki.seeedstudio.com/Seeeduino-XIAO/), 6 pots, 2 pushbuttons, 2 Midi DIN connectors and an audio output.   
The Seeeduino XIAO board carries the powerful ATSAMD21G18A-MU which is a low-power microcontroller and has 14 GPIO PINs, a DAC output and one UART interface. To avoid grounding loops the MIDI input is properly opto-isolated, as required by official specifications.    
There are other boards pin compatible with the XIAO, like the [Adafruit QT Py - SAMD21](https://www.adafruit.com/product/4600) and the [Adafruit QT Py ESP32-S2 WiFi](https://www.adafruit.com/product/5325).

**Bend multi to your needs**  
The strong point is that you can program your **multi** as you need, using the Arduino IDE. 
On the MIDI side, possible use includes advanced midi controlling, filtering and remapping as well as algorhitmic arpeggiators and aleatoric patch generators. It's also perfect to explore digital synthesis techniques. 
 
**Software**    
 There are already several usable software and the list is constantly expanding:    
       
- **drone** it's a six oscillators drone machine.   
- **synth_sequencer** three oscillators monophonic synth with embedded sequencer. It receives MIDI clock and note messages.
- **bytebeat player** allows to play algorithmic music with no instruments and no real oscillators but a math expression that creates an audio output waveform as a function of time, processed 8000 times per second. The expression has six parameters accessible through the **multi** knobs.   
- **midi converter** is a USB to 5 DIN bi-directional MIDI converter.   
- **midi metronome** takes the MIDI clock from the 5 DIN midi connector and generates a metronome click on the audio output, to allow acoustic musicians to synchronize their performance to an electronic setup.   
- **hardware test** if you have choosen the DIY kit you may want to use this software to test your build.   
- **blink** the ubiquitous blink sketch.   

 **Sound generation**    
Since the Xiao board has a DAC, **multi** can also be used to produce sounds using the [Mozzi sonification library](https://github.com/sensorium/Mozzi) written by Tim Barrass or accessing directly to the DAC output.
And if you ever wanted some ’80s-era distopia movie (background) music for your daily life, Tod Kurt has a [repository](https://github.com/todbot/mozzi_experiments) with some interesting sketches ready to run on the SAMD21.



