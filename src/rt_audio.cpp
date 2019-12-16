#include "rt_audio.h"

#include "rt_audio_converter.h"

int rt_callback(void *outputBuffer, void *inputBuffer, unsigned int nFrames, double streamTime, RtAudioStreamStatus status, void *userData)
{
	RtAudioWrap *wrap = (RtAudioWrap *)userData;

	std::shared_ptr<int8_t> inputData(new int8_t[wrap->_frameSize * wrap->_inputChannels * wrap->_sampleSize]);

	std::unique_lock lk(wrap->_outputDataMutex, std::defer_lock);

	// Verify frame size
	if (nFrames != wrap->_frameSize)
	{
		return 0;
	}

	// If the output channel live
	if (wrap->_outputChannels)
	{
		// Lock the output data queue mutex
		lk.lock();

		// If there is a new output data
		if (!wrap->_outputData.empty())
		{
			// Copy the new data to the output buffer
			memcpy(outputBuffer, wrap->_outputData.front().get(), wrap->_frameSize * wrap->_outputChannels * wrap->_sampleSize);

			// Remove the first element
			wrap->_outputData.pop();
		}

		// Unlock the output data queue mutex
		lk.unlock();
	}

	// If the input channel live
	if (wrap->_inputChannels)
	{
		// Copy the input data
		memcpy(inputData.get(), inputBuffer, wrap->_frameSize * wrap->_inputChannels * wrap->_sampleSize);

		// If the TSFN isn't null
		if (wrap->_inputTsfn != nullptr)
		{
			// Call the input callback
			wrap->_inputTsfn.BlockingCall([wrap, inputData](Napi::Env env, Napi::Function callback) {
				callback.Call({Napi::Buffer<int8_t>::Copy(env, inputData.get(), wrap->_frameSize * wrap->_inputChannels * wrap->_sampleSize)});
			});
		}
	}

	return 0;
}

Napi::Object RtAudioWrap::Init(Napi::Env env, Napi::Object exports)
{
	// Define the class and get it's ctor function
	Napi::Function ctor_func =
		DefineClass(env, "RtAudio",
					{InstanceMethod("openStream", &RtAudioWrap::openStream),
					 InstanceMethod("closeStream", &RtAudioWrap::closeStream),
					 InstanceMethod("isStreamOpen", &RtAudioWrap::isStreamOpen),
					 InstanceMethod("start", &RtAudioWrap::start),
					 InstanceMethod("stop", &RtAudioWrap::stop),
					 InstanceMethod("isStreamRunning", &RtAudioWrap::isStreamRunning),
					 InstanceMethod("write", &RtAudioWrap::write),
					 InstanceMethod("getApi", &RtAudioWrap::getApi),
					 InstanceMethod("getStreamLatency", &RtAudioWrap::getStreamLatency),
					 InstanceMethod("getStreamSampleRate", &RtAudioWrap::getStreamSampleRate),
					 InstanceMethod("getStreamTime", &RtAudioWrap::getStreamTime),
					 InstanceMethod("setStreamTime", &RtAudioWrap::setStreamTime),
					 InstanceMethod("getDevices", &RtAudioWrap::getDevices),
					 InstanceMethod("getDefaultInputDevice", &RtAudioWrap::getDefaultInputDevice),
					 InstanceMethod("getDefaultOutputDevice", &RtAudioWrap::getDefaultOutputDevice)});

	// Set the class's ctor function as a persistent object to keep it in memory
	constructor = Napi::Persistent(ctor_func);
	constructor.SuppressDestruct();

	// Export the ctor
	exports.Set("RtAudio", ctor_func);
	return exports;
}

RtAudioWrap::RtAudioWrap(const Napi::CallbackInfo &info) : Napi::ObjectWrap<RtAudioWrap>(info), _frameSize(0), _inputChannels(0)
{
	RtAudio::Api api = info.Length() == 0 ? RtAudio::Api::UNSPECIFIED : (RtAudio::Api)(int)info[0].As<Napi::Number>();

	try
	{
		// Init the RtAudio object with the wanted api
		_rtAudio = std::make_shared<RtAudio>(api);
	}
	catch (std::exception &ex)
	{
		throw Napi::Error::New(info.Env(), ex.what());
	}
}

RtAudioWrap::~RtAudioWrap()
{
	_rtAudio->closeStream();
}

Napi::Value RtAudioWrap::getDevices(const Napi::CallbackInfo &info)
{
	Napi::Array devicesArray;
	std::vector<RtAudio::DeviceInfo> devices;

	// Determine the number of devices available
	unsigned int deviceCount = _rtAudio->getDeviceCount();

	// Scan through devices for various capabilities
	RtAudio::DeviceInfo device;
	for (unsigned int i = 0; i < deviceCount; i++)
	{
		// Get the device's info
		device = _rtAudio->getDeviceInfo(i);

		// If the device is probed
		if (device.probed)
		{
			devices.push_back(device);
		}
	}

	// Allocate the devices array
	devicesArray = Napi::Array::New(info.Env(), devices.size());

	// Convert the devices to objects
	for (unsigned int i = 0; i < devices.size(); i++)
	{
		devicesArray[i] = RtAudioConverter::ConvertDeviceInfo(info.Env(), devices[i]);
	}

	return devicesArray;
}

Napi::Value RtAudioWrap::getDefaultInputDevice(const Napi::CallbackInfo &info)
{
	return Napi::Number::New(info.Env(), _rtAudio->getDefaultInputDevice());
}

Napi::Value RtAudioWrap::getDefaultOutputDevice(const Napi::CallbackInfo &info)
{
	return Napi::Number::New(info.Env(), _rtAudio->getDefaultOutputDevice());
}

void RtAudioWrap::openStream(const Napi::CallbackInfo &info)
{
	RtAudio::StreamParameters outputParams = info[0].IsNull() || info[0].IsUndefined() ? RtAudio::StreamParameters() : RtAudioConverter::ConvertStreamParameters(info[0].As<Napi::Object>());
	RtAudio::StreamParameters inputParams = info[1].IsNull() || info[1].IsUndefined() ? RtAudio::StreamParameters() : RtAudioConverter::ConvertStreamParameters(info[1].As<Napi::Object>());
	RtAudioFormat format = (int)info[2].As<Napi::Number>();
	unsigned int sampleRate = info[3].As<Napi::Number>();
	unsigned int frameSize = info[4].As<Napi::Number>();
	std::string streamName = info[5].As<Napi::String>();
	Napi::Function inputCallback = info[6].IsNull() || info[6].IsUndefined() ? Napi::Function() : info[6].As<Napi::Function>();
	RtAudioStreamFlags flags = info.Length() < 8 ? 0 : info[7].As<Napi::Number>();

	RtAudio::StreamOptions options;

	// If there is already a TSFN, release it
	if (_inputTsfn != nullptr)
	{
		_inputTsfn.Release();
	}

	// Save frame size
	_frameSize = frameSize;

	// Save input and output channels
	_inputChannels = inputParams.nChannels;
	_outputChannels = outputParams.nChannels;

	// Save the sample size by the format
	_sampleSize = getSampleSizeForFormat(format);

	// Set stream options
	options.flags = flags;
	options.streamName = streamName;

	try
	{
		// Open the stream
		_rtAudio->openStream(info[0].IsNull() || info[0].IsUndefined() ? nullptr : &outputParams, info[1].IsNull() || info[1].IsUndefined() ? nullptr : &inputParams, format, sampleRate, &frameSize, rt_callback, this, &options);
	}
	catch (std::exception &ex)
	{
		throw Napi::Error::New(info.Env(), ex.what());
	}

	// If the input callback isn't null and the input info isn't null
	if (!inputCallback.IsEmpty() && !info[1].IsNull() && !info[1].IsUndefined())
	{
		// Save the input callback as a thread safe function
		_inputTsfn = Napi::ThreadSafeFunction::New(info.Env(), inputCallback, "inputCallback", 0, 1, [this](Napi::Env) {});
	}
}

void RtAudioWrap::closeStream(const Napi::CallbackInfo &info)
{
	_rtAudio->closeStream();
}

Napi::Value RtAudioWrap::isStreamOpen(const Napi::CallbackInfo &info)
{
	return Napi::Boolean::New(info.Env(), _rtAudio->isStreamOpen());
}

void RtAudioWrap::start(const Napi::CallbackInfo &info)
{
	try
	{
		// Start the stream
		_rtAudio->startStream();
	}
	catch (std::exception &ex)
	{
		throw Napi::Error::New(info.Env(), ex.what());
	}
}

void RtAudioWrap::stop(const Napi::CallbackInfo &info)
{
	try
	{
		// Stop the stream
		_rtAudio->stopStream();
	}
	catch (std::exception &ex)
	{
		throw Napi::Error::New(info.Env(), ex.what());
	}
}

Napi::Value RtAudioWrap::isStreamRunning(const Napi::CallbackInfo &info)
{
	return Napi::Boolean::New(info.Env(), _rtAudio->isStreamRunning());
}

void RtAudioWrap::write(const Napi::CallbackInfo &info)
{
	Napi::Buffer buf = info[0].As<Napi::Buffer<uint8_t>>();

	std::shared_ptr<int8_t> data;

	std::unique_lock lk(_outputDataMutex, std::defer_lock);

	// Check for valid size of the pcm data
	if (buf.Length() != _frameSize * _outputChannels * _sampleSize)
	{
		throw Napi::Error::New(info.Env(), "Invalid size of the PCM data!");
	}

	// Allocate buffer for the PCM data
	data = std::shared_ptr<int8_t>(new int8_t[_frameSize * _outputChannels * _sampleSize]);

	// Copy the data to the new buffer
	memcpy(data.get(), buf.Data(), _frameSize * _outputChannels * _sampleSize);

	// Lock the output data queue mutex
	lk.lock();

	// Push the new PCM data to the queue
	_outputData.push(data);

	// Unlock the output data queue mutex
	lk.unlock();
}

Napi::Value RtAudioWrap::getApi(const Napi::CallbackInfo &info)
{
	return Napi::String::New(info.Env(), _rtAudio->getApiDisplayName(_rtAudio->getCurrentApi()));
}

Napi::Value RtAudioWrap::getStreamLatency(const Napi::CallbackInfo &info)
{
	try
	{
		return Napi::Number::New(info.Env(), _rtAudio->getStreamLatency());
	}
	catch (std::exception &ex)
	{
		throw Napi::Error::New(info.Env(), ex.what());
	}
}

Napi::Value RtAudioWrap::getStreamSampleRate(const Napi::CallbackInfo &info)
{
	try
	{
		return Napi::Number::New(info.Env(), _rtAudio->getStreamSampleRate());
	}
	catch (std::exception &ex)
	{
		throw Napi::Error::New(info.Env(), ex.what());
	}
}

Napi::Value RtAudioWrap::getStreamTime(const Napi::CallbackInfo &info)
{
	try
	{
		return Napi::Number::New(info.Env(), _rtAudio->getStreamTime());
	}
	catch (std::exception &ex)
	{
		throw Napi::Error::New(info.Env(), ex.what());
	}
}

void RtAudioWrap::setStreamTime(const Napi::CallbackInfo &info)
{
	double time = info[0].As<Napi::Number>();

	try
	{
		_rtAudio->setStreamTime(time);
	}
	catch (std::exception &ex)
	{
		throw Napi::Error::New(info.Env(), ex.what());
	}
}

unsigned int RtAudioWrap::getSampleSizeForFormat(RtAudioFormat format)
{
	switch (format)
	{
	case RTAUDIO_SINT8:
		return 1;

	case RTAUDIO_SINT16:
		return 2;

	case RTAUDIO_SINT24:
		return 3;

	case RTAUDIO_SINT32:
	case RTAUDIO_FLOAT32:
		return 4;

	case RTAUDIO_FLOAT64:
		return 8;

	default:
		return 0;
	}
}