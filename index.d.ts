/// <reference types="node" />

/** Opus Coding Mode. */
declare const enum OpusApplication {
  /** Best for most VoIP/VideoConference applications where listening quality and intelligibility matter most. */
  OPUS_APPLICATION_VOIP = 2048,

  /** Best for broadcast/high-fidelity application where the decoded audio should be as close as possible to the input. */
  OPUS_APPLICATION_AUDIO = 2049,

  /** Only use when lowest-achievable latency is what matters most. Voice-optimized modes cannot be used. */
  OPUS_APPLICATION_RESTRICTED_LOWDELAY = 2051,
}

/** Audio API specifier arguments. */
declare const enum RtAudioApi {
  /** Search for a working compiled API. */
  UNSPECIFIED,

  /** Macintosh OS-X Core Audio API. */
  MACOSX_CORE,

  /** The Advanced Linux Sound Architecture API. */
  LINUX_ALSA,

  /** The Jack Low-Latency Audio Server API. */
  UNIX_JACK,

  /** The Linux PulseAudio API. */
  LINUX_PULSE,

  /** The Linux Open Sound System API. */
  LINUX_OSS,

  /** The Steinberg Audio Stream I/O API. */
  WINDOWS_ASIO,

  /** The Microsoft WASAPI API. */
  WINDOWS_WASAPI,

  /** The Microsoft DirectSound API. */
  WINDOWS_DS,

  /** A compilable but non-functional API. */
  RTAUDIO_DUMMY,
}

/** The format of the PCM data. */
declare const enum RtAudioFormat {
  /** 8-bit signed integer. */
  RTAUDIO_SINT8 = 0x1,

  /** 16-bit signed integer. */
  RTAUDIO_SINT16 = 0x2,

  /** 24-bit signed integer - Removed. */
  RTAUDIO_SINT24 = 0x4,

  /** 32-bit signed integer. */
  RTAUDIO_SINT32 = 0x8,

  /** Normalized between plus/minus 1.0. */
  RTAUDIO_FLOAT32 = 0x10,

  /** Normalized between plus/minus 1.0. */
  RTAUDIO_FLOAT64 = 0x20,
}

/** Flags that change the default stream behavior */
declare const enum RtAudioStreamFlags {
  /** Use non-interleaved buffers (default = interleaved). */
  RTAUDIO_NONINTERLEAVED = 0x1,

  /** Attempt to set stream parameters for lowest possible latency. */
  RTAUDIO_MINIMIZE_LATENCY = 0x2,

  /** Attempt grab device and prevent use by others. */
  RTAUDIO_HOG_DEVICE = 0x4,

  /** Try to select realtime scheduling for callback thread. */
  RTAUDIO_SCHEDULE_REALTIME = 0x8,

  /** Use the "default" PCM device (ALSA only). */
  RTAUDIO_ALSA_USE_DEFAULT = 0x10,

  /** Do not automatically connect ports (JACK only). */
  RTAUDIO_JACK_DONT_CONNECT = 0x20,
}

/** RtAudio error types */
declare const enum RtAudioErrorType {
  /** A non-critical error. */
  WARNING,

  /** A non-critical error which might be useful for debugging. */
  DEBUG_WARNING,

  /** The default, unspecified error type. */
  UNSPECIFIED,

  /** No devices found on system. */
  NO_DEVICES_FOUND,

  /** An invalid device ID was specified. */
  INVALID_DEVICE,

  /** An error occurred during memory allocation. */
  MEMORY_ERROR,

  /** An invalid parameter was specified to a function. */
  INVALID_PARAMETER,

  /** The function was called incorrectly. */
  INVALID_USE,

  /** A system driver error occurred. */
  DRIVER_ERROR,

  /** A system error occurred. */
  SYSTEM_ERROR,

  /** A thread error occurred. */
  THREAD_ERROR,
}

/** The public device information structure for returning queried values. */
declare interface RtAudioDeviceInfo {
  /** Unique numeric device identifier. */
  id: number;

  /** Character string device identifier. */
  name: string;

  /** Maximum output channels supported by device. */
  outputChannels: number;

  /** Maximum input channels supported by device. */
  inputChannels: number;

  /** Maximum simultaneous input/output channels supported by device. */
  duplexChannels: number;

  /** Is the device the default output device */
  isDefaultOutput: number;

  /** Is the device the default input device */
  isDefaultInput: number;

  /** Supported sample rates (queried from list of standard rates). */
  sampleRates: Array<number>;

  /** Preferred sample rate, e.g. for WASAPI the system sample rate. */
  preferredSampleRate: number;

  /** Bit mask of supported data formats. */
  nativeFormats: number;
}

/** The structure for specifying input or ouput stream parameters. */
declare interface RtAudioStreamParameters {
  /**
   * Device id. Can be obtained using `getDefaultInputDevice`/`getDefaultOutputDevice` or using `getDevices` from the field `id`.
   *
   * NOTE: For legacy reasons, this field also accepts the index of the device in the array that is returned from `getDevices`. Please avoid using it.
   */
  deviceId?: number;

  /** Number of channels. */
  nChannels: number;

  /** First channel index on device (default = 0). */
  firstChannel?: number;
}

/**
 * A class that encodes PCM input signal from 16-bit signed integer or floating point input.
 */
export declare class OpusEncoder {
  /** The bitrate of the encode. */
  public bitrate: number;

  /**
   * Create an opus encoder.
   * @param sampleRate Sampling rate of input signal (Hz) This must be one of 8000, 12000, 16000, 24000, or 48000.
   * @param channels Number of channels (1 or 2) in input signal.
   * @param application Coding mode.
   */
  constructor(
    sampleRate: number,
    channels: number,
    application: OpusApplication
  );

  /**
   * Encodes an Opus frame from 16-bit signed integer input.
   * @param pcm PCM input signal buffer. Length is frame_size * channels * 2.
   * @param frameSize Number of samples per channel in the input signal. This must be an Opus frame size for the encoder's sampling rate.
   * @return The encoded Opus packet of the frame.
   */
  public encode(pcm: Buffer, frameSize: number): Buffer;

  /**
   * Encodes an Opus frame from floating point input.
   * @param pcm PCM input signal buffer. Length is frame_size * channels * 2.
   * @param frameSize Number of samples per channel in the input signal. This must be an Opus frame size for the encoder's sampling rate.
   * @return The encoded Opus packet of the frame.
   */
  public encodeFloat(pcm: Buffer, frameSize: number): Buffer;
}

/**
 * A class that decodes Opus packet to 16-bit signed integer or floating point PCM.
 */
export declare class OpusDecoder {
  /**
   * Create an opus decoder.
   * @param sampleRate Sample rate to decode at (Hz). This must be one of 8000, 12000, 16000, 24000, or 48000.
   * @param channels Number of channels (1 or 2) to decode.
   */
  constructor(sampleRate: number, channels: number);

  /**
   * Decodes an Opus packet to 16-bit signed integer PCM.
   * @param data The data of the opus packet to decode.
   * @param frameSize Number of samples per channel in the opus packet. This must be an Opus frame size for the encoder's sampling rate.
   * @return The output signal in 16-bit signed integer PCM.
   */
  public decode(data: Buffer, frameSize: number): Buffer;

  /**
   * Decodes an Opus packet to floating point PCM.
   * @param data The data of the opus packet to decode.
   * @param frameSize Number of samples per channel in the opus packet. This must be an Opus frame size for the encoder's sampling rate.
   * @return The output signal in floating point PCM.
   */
  public decodeFloat(data: Buffer, frameSize: number): Buffer;
}

/** RtAudio provides a common API (Application Programming Interface)
    for realtime audio input/output across Linux (native ALSA, Jack,
    and OSS), Macintosh OS X (CoreAudio and Jack), and Windows
    (DirectSound, ASIO and WASAPI) operating systems. */
export declare class RtAudio {
  /** The volume of the output device. This should be a number between 0 and 1. */
  public outputVolume: number;

  /** The number of elapsed seconds since the stream was started. This should be a time in seconds greater than or equal to 0.0.  */
  public streamTime: number;

  /**
   * Create an RtAudio instance.
   * @param api The audio API to use. (Default will be automatically selected)
   */
  constructor(api?: RtAudioApi);

  /**
   * A public function for opening a stream with the specified parameters. Returns the actual frameSize used by the stream, useful if a frameSize of 0 is passed.
   * @param outputParameters Specifies output stream parameters to use when opening a stream. For input-only streams, this argument should be null.
   * @param inputParameters Specifies input stream parameters to use when opening a stream. For output-only streams, this argument should be null.
   * @param format An RtAudio.Format specifying the desired sample data format.
   * @param sampleRate The desired sample rate (sample frames per second).
   * @param frameSize The amount of samples per frame. Can be 0 for some APIs, in which case the lowest allowable value is determined; this is necessary for the ASIO & Jack APIs where the user can set an overriding global buffer size for their device.
   * @param streamName A stream name (currently used only in Jack).
   * @param inputCallback A callback that is called when a new input signal is available. Should be null for output-only streams.
   * @param frameOutputCallback A callback that is called when a frame is finished playing in the output device.
   * @param flags A bit-mask of stream flags (RtAudio.StreamFlags).
   * @param errorCallback A callback that is called when an error has occurred.
   * @return The actual frame-size used for stream. Useful if passed 0 as frameSize.
   */
  openStream(
    outputParameters: RtAudioStreamParameters | null,
    inputParameters: RtAudioStreamParameters | null,
    format: RtAudioFormat,
    sampleRate: number,
    frameSize: number,
    streamName: string,
    inputCallback: ((inputData: Buffer) => void) | null,
    frameOutputCallback: (() => void) | null,
    flags?: RtAudioStreamFlags,
    errorCallback?: ((type: RtAudioErrorType, msg: string) => void) | null
  ): number;

  /**
   * A function that closes a stream and frees any associated stream memory.
   */
  closeStream(): void;

  /**
   * Returns true if a stream is open and false if not.
   */
  isStreamOpen(): boolean;

  /**
   * Start the stream.
   */
  start(): void;

  /**
   * Stop the stream.
   */
  stop(): void;

  /**
   * Returns true if the stream is running and false if it is stopped or not open.
   */
  isStreamRunning(): boolean;

  /**
   * Queues a new output PCM data to be played using the stream.
   * @param pcm The raw PCM data. The length should be frame_size * no_of_output_channels * size_of_sample.
   */
  write(pcm: Buffer): void;

  /**
   * Clears the output stream queue.
   */
  clearOutputQueue(): void;

  /**
   * Returns the full display name of the current used API.
   */
  getApi(): string;

  /**
   * Returns the internal stream latency in sample frames.
   */
  getStreamLatency(): number;

  /**
   * Returns actual sample rate in use by the stream.
   */
  getStreamSampleRate(): number;

  /**
   * Returns the list of available devices.
   */
  getDevices(): Array<RtAudioDeviceInfo>;

  /**
   * Returns the id of the default input device.
   */
  getDefaultInputDevice(): number;

  /**
   * Returns the id of the default output device.
   */
  getDefaultOutputDevice(): number;

  /**
   * Sets the input callback function for the input device.
   * @param callback A callback that is called when a new input signal is available. Should be null for output-only streams.
   */
  setInputCallback(callback: ((inputData: Buffer) => void) | null): void;

  /**
   * Sets the frame output playback for the output device.
   * @param callback A callback that is called when a frame is finished playing in the output device.
   */
  setFrameOutputCallback(callback: (() => void) | null): void;
}
