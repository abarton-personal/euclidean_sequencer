# Euclidean Sequencer
A MIDI sequencer I built with an ESP32! Complex rhythms can be programmed in real time using only a rotary knob and a few buttons. It connects to your DAW via Bluetooth. Timing can be controlled by the ESP32's internal clock, or it can remotely sync with the DAW's clock.

A [Euclidean rhythm](https://en.wikipedia.org/wiki/Euclidean_rhythm) is any rhythm whose beats and rests are evenly spaced throughout the measure (or rounded to the nearest 16th note if the number can't be divided evenly). I was originally inspired by the [Qu-bit pulsar](https://modulargrid.net/e/qu-bit-electronix-pulsar), and since I don't have a modular synth I decided to build a standalone MIDI based sequencer.

Here is a little demo I did:
https://drive.proton.me/urls/Y95E5P4F14#9NDDEcJm66s1

## Setup
NOTE: I've only tested this on Mac with Ableton Live. It should work with any DAW, and I think on Windows, but unfortunately I can't provide instructions for anything else. If you're one the 4 people who will read this, and you have a different setup, reach out and I will try to help you get connected (and then I'll update these instructions).

1. Plug the USB cable into the microcontroller's USB port on the top left. The device will power on and do a little dance with the lights. It will immediately start broadcasting its Bluetooth pairing availability.
### Mac
2. Open **Audio MIDI Setup.app**
3. Click the **Configure Bluetooth** icon on the top right of the **MIDI Studio** window to scan for Bluetooth devices.
4. Hopefully **"Euclidean Sequencer"** appears as one of the available devices. Click Connect.
### Ableton
5. Go to **Settings** -> **Link, Tempo & MIDI**
6. Click one of the **Input Devices** dropdown boxes and select Euclidean Sequencer
7. Now the Euclidean Sequencer should be able to send note on and off messages like any other MIDI controller. Back in arrangement/session view, select "Euclidean Sequencer (bluetooth)" as the input device to one of your MIDI tracks, drag a drum kit onto it, and arm the track.

## Usage
### Modes
There are currently 3 modes of operation. Press the **Mode** button to cycle through them. An abbreviated mode name should appear on the seven segment display.
#### 1. EuCL (Euclidean)
Euclidean mode is where you set the notes. Notice there are 16 lights in the light wheel. Each light represents a 16th note in a 4/4 measure. The top one is the downbeat, far right is beat 2, etc.
- Rotate the knob to increase or decrease the number of audible beats in the measure. They will be automatically evenly distributed throughout the measure (rounded to the nearest 16th note).
- Press down on the knob until it clicks, hold it down and rotate. Any notes that are already set will rotate with it. This allows you to set them to off-beats.
#### 2. rAtE (tempo)
I called it rate because I couldn't do an M on the seven segment display, and "tEPo" sounds stupid. Dont @ me. 
In rAtE mode, use the knob to adjust the tempo. Default is 120.
#### 3. ShuF (swing)
In ShuF mode, rotate the knob to set the heaviness of the swing. 
- A swing of 0 means straight 16ths.
- 2 is about 66/33 or a nice triplet swing
- 4 is 100/0 swing, aka basically every other one gets eaten. Not very pleasant
- You can also do negative swing, which is another thing I have invented that is also not very pleasant, but it's there because why not


### Channels
- Press the **channel button** to cycle through channels. I call them channels, but technically they all correspond to the same MIDI channel 0, and just play different notes. 
    - Ch 0 = MIDI note 36 = C2 = usually the kick drum on drum pads.
    - Ch 1 = MIDI note 37 = C#2 = usually the side stick
    - Ch 2 = D2 = usually snare
    - etc
- press and hold the channel button, then rotate the knob to increase or decrease the TOTAL number of channels. By default there are 4. You can have up to 8.

### Start/Stop
Press the start/stop button to start and stop playback using the Euclidean Sequencer's internal clock.
### Syncing with DAW
Optionally, you can sync the device's playback timing with your DAW.
1. In Ableton, go to **Settings** -> **Link, Tempo & MIDI**
2. Scroll down to **Output Ports**, find Euclidean Sequencer, and check both the "Track" and "Sync" boxes.
3. Click the little arrow to expand more options.
    a. MIDI clock type: Song
    b. Hardware Resync: Stop and Start
    c. MIDI Clock Sync Delay: It takes some time to send the clock messages to the device and then for the device to send the note messages back to the DAW. For me around -60ms to -80ms works best, but will vary from computer to computer.

Now the device will start and stop playback with the DAW, and share the tempo and measure counter.

Note: In this configuration, the Start/Stop button on the Sequencer is disabled, and if you try to press it, it will just display the word "Sync" to remind you that it is being controlled externally.

## Misc Notes
#### Instruments
- I stick to drums. You can also assign any other MIDI instrument like a piano I suppose, but it sounds pretty terrible. In the future I might figure out a way to dynamically assign a specific note to each channel. But it is not this day.
- Sometimes MIDI drum kits have a thing called "choke", which prevents certain drums from being played at the same time. It makes sense, if you play a closed hi-hat, then it isn't open anymore. But for a weird controller such as this, this could be a bother. To disable it:
    - (In Ableton) Just to the left of the drum rack grid there is a tiny button called "Show/Hide Chain List". Click it.
    - That will reveal another tiny button below it called "Show/Hide Input/Output Section". Click it.
    - Now to the right of the drum rack grid there should be a "Chain List View" with a column that says "Choke". Set all drums to "None"

#### Time Signatures
It doesn't really know how to handle anything other than 4/4. 

#### USB
The Euclidean Sequencer only communicates over Bluetooth. Which is kind of annoying because you _already_ have to plug in a USB cable, so why not use that? Unfortunately it's a hardware limitation of the particular ESP32 kit I designed the board around, it receives power through the cable but has no USB interface. Other ESP32 variations exist that can communicate over USB, but they are not pin-compatible, so I would have to redesign the layout of the board, which may or may not ever happen.
#### Feedback
I would love to hear your feedback. If you have one of these and have some ideas, you know where to find me!


