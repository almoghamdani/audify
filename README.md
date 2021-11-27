[![npm version](https://badge.fury.io/js/audify.svg)](https://badge.fury.io/js/audify)
[![Build Status](https://travis-ci.com/almoghamdani/audify.svg?branch=master)](https://travis-ci.com/almoghamdani/audify)

# Audify.js
Audify.js - Play/Stream/Record PCM audio data &amp; Encode/Decode Opus to PCM audio data

## Features
* Encode 16-bit integer PCM or floating point PCM to Opus packet using C++ Opus library.
* Decode Opus packets to 16-bit integer PCM or floating point PCM using C++ Opus library.
* Complete API for realtime audio input/output across Linux (native ALSA, JACK, PulseAudio and OSS), Macintosh OS X (CoreAudio and JACK), and Windows (DirectSound, ASIO and WASAPI) operating systems using C++ RtAudio library.

## Installation
```
npm install audify
```

***Most regular installs will support prebuilds that are built with each release.***
***The prebuilds are for Node v10.17.x+, v12.11.x+, v13.x.x, v14.x.x, v15.x.x, v16.x.x and Electron v8.x.x, v9.x.x, v10.x.x, v11.x.x, v12.x.x., v13.x.x., v14.x.x.***

#### Requirements for source build

* Node or Electron versions that support N-API 5 and up ([N-API Node Version Matrix](https://nodejs.org/docs/latest/api/n-api.html#node-api-version-matrix))
* [CMake](http://www.cmake.org/download/)
* A proper C/C++ compiler toolchain of the given platform
    * **Windows**:
        * [Visual C++ Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/) or a recent version of Visual C++ will do ([the free Community](https://www.visualstudio.com/products/visual-studio-community-vs) version works well)
    * **Unix/Posix**:
        * Clang or GCC
        * Ninja or Make (Ninja will be picked if both present)

## Example
#### Opus Encode & Decode
```javascript
const { OpusEncoder, OpusDecoder, OpusApplication } = require("audify");

// Init encoder and decoder
// Sample rate is 48kHz and the amount of channels is 2
// The opus coding mode is optimized for audio
const encoder = new OpusEncoder(48000, 2, OpusApplication.OPUS_APPLICATION_AUDIO);
const decoder = new OpusDecoder(48000, 2);

const frameSize = 1920; // 40ms
const buffer = ...

// Encode and then decode
var encoded = encoder.encode(buffer, frameSize);
var decoded = decoder.decode(encoded, frameSize);
```

#### Record audio and play it back realtime
```javascript
const { RtAudio, RtAudioFormat } = require("audify");

// Init RtAudio instance using default sound API
const rtAudio = new RtAudio(/* Insert here specific API if needed */);

// Open the input/output stream
rtAudio.openStream(
	{ deviceId: rtAudio.getDefaultOutputDevice(), // Output device id (Get all devices using `getDevices`)
	  nChannels: 1, // Number of channels
	  firstChannel: 0 // First channel index on device (default = 0).
	},
	{ deviceId: rtAudio.getDefaultInputDevice(), // Input device id (Get all devices using `getDevices`)
	  nChannels: 1, // Number of channels
	  firstChannel: 0 // First channel index on device (default = 0).
	},
	RtAudioFormat.RTAUDIO_SINT16, // PCM Format - Signed 16-bit integer
	48000, // Sampling rate is 48kHz
	1920, // Frame size is 1920 (40ms)
	"MyStream", // The name of the stream (used for JACK Api)
	pcm => rtAudio.write(pcm) // Input callback function, write every input pcm data to the output buffer
);

// Start the stream
rtAudio.start();
```

## Documentation
Full documentation available [here](https://almoghamdani.github.io/audify/).

## Legal
This project is licensed under the MIT license.
