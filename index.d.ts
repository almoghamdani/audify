namespace Audify {
	/** Opus Coding Mode. */
	enum OpusApplication {
		/** Best for most VoIP/videoconference applications where listening quality and intelligibility matter most. */
		OPUS_APPLICATION_VOIP = 2048,

		/** Best for broadcast/high-fidelity application where the decoded audio should be as close as possible to the input. */
		OPUS_APPLICATION_AUDIO = 2049,

		/** Only use when lowest-achievable latency is what matters most. Voice-optimized modes cannot be used. */
		OPUS_APPLICATION_RESTRICTED_LOWDELAY = 2051
	}

	/** Options for Opus Encoder. */
	interface OpusEncoderOptions {
		/** Sampling rate of input signal (Hz) This must be one of 8000, 12000, 16000, 24000, or 48000. */
		readonly sampleRate: number;

		/** Number of channels (1 or 2) in input signal. */
		readonly channels: number;

		/** Coding mode. */
		readonly application: OpusApplication;
	}

	/** Options for Opus Decoder. */
	interface OpusDecoderOptions {
		/** Sample rate to decode at (Hz). This must be one of 8000, 12000, 16000, 24000, or 48000. */
		readonly sampleRate: number;

		/** Number of channels (1 or 2) to decode. */
		readonly channels: number;
	}
}

/**
 * A class that encodes PCM input signal from 16-bit signed integer or floating point input.
 */
class OpusEncoder {
	/* The bitrate of the encode */
	public bitrate: number;

	/**
	 * Create an opus encoder.
	 * @param opts The options for the opus encoder.
	 */
	constructor(opts: Audify.OpusEncoderOptions);

	/**
	 * Encodes an Opus frame from 16-bit signed integer input.
	 * @param pcm PCM input signal buffer. Length is frame_size * channels * 2.
	 * @param frameSize Number of samples per channel in the input signal. This must be an Opus frame size for the encoder's sampling rate.
	 * @return The encoded Opus packet of the frame.
	 */
	public encode(pcm: BufferSource, frameSize: number): BufferSource;

	/**
	 * Encodes an Opus frame from floating point input.
	 * @param pcm PCM input signal buffer. Length is frame_size * channels * 2.
	 * @param frameSize Number of samples per channel in the input signal. This must be an Opus frame size for the encoder's sampling rate.
	 * @return The encoded Opus packet of the frame.
	 */
	public encodeFloat(pcm: BufferSource, frameSize: number): BufferSource;
}

/**
 * A class that decodes Opus packet to 16-bit signed integer or floating point PCM.
 */
class OpusDecoder {
	/**
	 * Create an opus decoder.
	 * @param opts The options for the opus decoder.
	 */
	constructor(opts: Audify.OpusDecoderOptions);

	/**
	 * Decodes an Opus packet to 16-bit signed integer PCM.
	 * @param data The data of the opus packet to decode.
	 * @param frameSize Number of samples per channel in the opus packet. This must be an Opus frame size for the encoder's sampling rate.
	 * @return The output signal in 16-bit signed integer PCM.
	 */
	public decode(data: BufferSource, frameSize: number): BufferSource;

	/**
	 * Decodes an Opus packet to floating point PCM.
	 * @param data The data of the opus packet to decode.
	 * @param frameSize Number of samples per channel in the opus packet. This must be an Opus frame size for the encoder's sampling rate.
	 * @return The output signal in floating point PCM.
	 */
	public encodeFloat(data: BufferSource, frameSize: number): BufferSource;
}

exports = { OpusEncoder, OpusDecoder };
