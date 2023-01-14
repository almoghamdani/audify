#include "rt_audio.h"

#include <cmath>
#include <sstream>

#include "rt_audio_converter.h"

Napi::ThreadSafeFunction errorTsfn;

int rt_callback(void* outputBuffer,
                void* inputBuffer,
                unsigned int nFrames,
                double streamTime,
                RtAudioStreamStatus status,
                void* userData)
{
    const auto wrap = static_cast<RtAudioWrap*>(userData);

    std::shared_ptr<int8_t> inputData(
        new int8_t[wrap->_frameSize * wrap->_inputChannels * wrap->_sampleSize]);

    std::unique_lock outputLock(wrap->_outputDataMutex, std::defer_lock);
    std::unique_lock tsfnLock(wrap->_tsfnMutex, std::defer_lock);

    // Verify frame size
    if (nFrames != wrap->_frameSize)
    {
        return 0;
    }

    // If the output channel live
    if (wrap->_outputChannels)
    {
        // Lock the output data queue mutex and threadsafe functions lock
        std::lock(outputLock, tsfnLock);

        // If there is a new output data
        if (!wrap->_outputData.empty())
        {
            // Copy the new data to the output buffer and apply volume
            wrap->applyVolume(wrap->_outputData.front().get(),
                              outputBuffer,
                              wrap->_frameSize * wrap->_outputChannels);

            // Remove the first element
            wrap->_outputData.pop();

            // If the frame output threadsafe-function isn't null
            if (wrap->_frameOutputTsfn != nullptr)
            {
                // Call the frame output callback
                wrap->_frameOutputTsfn.NonBlockingCall(
                    [](Napi::Env, Napi::Function callback) { callback.Call({}); });
            }
        }
        else
        {
            // Clear the output buffer
            memset(outputBuffer,
                   0,
                   wrap->_frameSize * wrap->_outputChannels * wrap->_sampleSize);
        }

        // Unlock the output data queue mutex
        outputLock.unlock();
    }

    // If the input channel live
    if (wrap->_inputChannels)
    {
        // If the threadsafe mutex isn't locked, lock it
        if (!tsfnLock.owns_lock())
        {
            tsfnLock.lock();
        }

        // If the input threadsafe-function isn't null
        if (wrap->_inputTsfn != nullptr)
        {
            // Copy the input data
            memcpy(inputData.get(),
                   inputBuffer,
                   wrap->_frameSize * wrap->_inputChannels * wrap->_sampleSize);

            // Call the input callback
            wrap->_inputTsfn.NonBlockingCall(
                [wrap, inputData](Napi::Env env, Napi::Function callback)
                {
                    callback.Call({
                        Napi::Buffer<int8_t>::Copy(
                            env,
                            inputData.get(),
                            wrap->_frameSize * wrap->_inputChannels * wrap->_sampleSize)
                    });
                });
        }
    }

    return 0;
}

Napi::Object RtAudioWrap::Init(Napi::Env env, Napi::Object exports)
{
    // Define the class and get it's ctor function
    const Napi::Function ctor_func = DefineClass(
        env,
        "RtAudio",
        {
            InstanceMethod("openStream", &RtAudioWrap::openStream),
            InstanceMethod("closeStream", &RtAudioWrap::closeStream),
            InstanceMethod("isStreamOpen", &RtAudioWrap::isStreamOpen),
            InstanceMethod("start", &RtAudioWrap::start),
            InstanceMethod("stop", &RtAudioWrap::stop),
            InstanceMethod("isStreamRunning", &RtAudioWrap::isStreamRunning),
            InstanceMethod("write", &RtAudioWrap::write),
            InstanceMethod("clearOutputQueue", &RtAudioWrap::clearOutputQueue),
            InstanceMethod("getApi", &RtAudioWrap::getApi),
            InstanceMethod("getStreamLatency", &RtAudioWrap::getStreamLatency),
            InstanceMethod("getStreamSampleRate", &RtAudioWrap::getStreamSampleRate),
            InstanceMethod("getDevices", &RtAudioWrap::getDevices),
            InstanceMethod("getDefaultInputDevice", &RtAudioWrap::getDefaultInputDevice),
            InstanceMethod("getDefaultOutputDevice", &RtAudioWrap::getDefaultOutputDevice),
            InstanceMethod("setInputCallback", &RtAudioWrap::setInputCallback),
            InstanceMethod("setFrameOutputCallback", &RtAudioWrap::setFrameOutputCallback),
            InstanceAccessor("streamTime", &RtAudioWrap::getStreamTime, &RtAudioWrap::setStreamTime),
            InstanceAccessor("outputVolume", &RtAudioWrap::getOutputVolume, &RtAudioWrap::setOutputVolume)
        });

    // Set the class's ctor function as a persistent object to keep it in memory
    constructor = Napi::Persistent(ctor_func);
    constructor.SuppressDestruct();

    // Export the ctor
    exports.Set("RtAudio", ctor_func);
    return exports;
}

void RtAudioWrap::Destroy()
{
    constructor.Reset();

    if (errorTsfn != nullptr)
    {
        errorTsfn.Release();
        errorTsfn = nullptr;
    }
}

RtAudioWrap::RtAudioWrap(const Napi::CallbackInfo& info)
    :
    Napi::ObjectWrap<RtAudioWrap>(info),
    _frameSize(0),
    _inputChannels(0),
    _outputMultiplier(1),
    _outputVolume(1)
{
    RtAudio::Api api = info.Length() == 0
                           ? RtAudio::Api::UNSPECIFIED
                           : static_cast<RtAudio::Api>(static_cast<int>(info[0].As<Napi::Number>()));

    try
    {
        // Init the RtAudio object with the wanted api
        _rtAudio = std::make_shared<RtAudio>(
            api,
            [](RtAudioErrorType type, const std::string& errorMsg)
            {
                if (errorTsfn != nullptr)
                {
                    errorTsfn.NonBlockingCall(
                        [type, errorMsg](Napi::Env env, Napi::Function callback)
                        {
                            callback.Call({
                                Napi::Number::New(env, type),
                                Napi::String::New(env, errorMsg)
                            });
                        });
                }
            });
    }
    catch (std::exception& ex)
    {
        throw Napi::Error::New(info.Env(), ex.what());
    }
}

RtAudioWrap::~RtAudioWrap() { _rtAudio->closeStream(); }

Napi::Value RtAudioWrap::getDevices(const Napi::CallbackInfo& info)
{
    std::vector<RtAudio::DeviceInfo> devices;

    const std::vector<unsigned int> deviceIds = _rtAudio->getDeviceIds();
    for (const auto& deviceId : deviceIds)
    {
        devices.push_back(_rtAudio->getDeviceInfo(deviceId));
    }

    // Allocate the devices array
    Napi::Array devicesArray = Napi::Array::New(info.Env(), devices.size());

    // Convert the devices to objects
    for (unsigned int i = 0; i < devices.size(); i++)
    {
        devicesArray[i] = RtAudioConverter::ConvertDeviceInfo(info.Env(), devices[i]);
    }

    return devicesArray;
}

Napi::Value RtAudioWrap::getDefaultInputDevice(const Napi::CallbackInfo& info)
{
    return Napi::Number::New(info.Env(), _rtAudio->getDefaultInputDevice());
}

Napi::Value RtAudioWrap::getDefaultOutputDevice(
    const Napi::CallbackInfo& info)
{
    return Napi::Number::New(info.Env(), _rtAudio->getDefaultOutputDevice());
}

Napi::Value RtAudioWrap::openStream(const Napi::CallbackInfo& info)
{
    RtAudio::StreamParameters outputParams =
            info[0].IsNull() || info[0].IsUndefined()
                ? RtAudio::StreamParameters()
                : RtAudioConverter::ConvertStreamParameters(
                    info[0].As<Napi::Object>(),
                    false,
                    *_rtAudio);

    RtAudio::StreamParameters inputParams =
            info[1].IsNull() || info[1].IsUndefined()
                ? RtAudio::StreamParameters()
                : RtAudioConverter::ConvertStreamParameters(
                    info[1].As<Napi::Object>(),
                    true,
                    *_rtAudio);

    RtAudioFormat format = static_cast<int>(info[2].As<Napi::Number>());
    unsigned int sampleRate = info[3].As<Napi::Number>();
    unsigned int frameSize = info[4].As<Napi::Number>();
    std::string streamName = info[5].As<Napi::String>();
    Napi::Function inputCallback = info[6].IsNull() || info[6].IsUndefined()
                                       ? Napi::Function()
                                       : info[6].As<Napi::Function>();
    Napi::Function frameOutputCallback = info[7].IsNull() || info[7].IsUndefined()
                                             ? Napi::Function()
                                             : info[7].As<Napi::Function>();
    RtAudioStreamFlags flags = info.Length() < 9 ? 0 : info[8].As<Napi::Number>();
    Napi::Function errorCallback =
            info.Length() < 10 || info[9].IsNull() || info[9].IsUndefined()
                ? Napi::Function()
                : info[9].As<Napi::Function>();

    RtAudio::StreamOptions options;

    std::unique_lock tsfnLock(_tsfnMutex, std::defer_lock);

    // Set SINT24 as invalid
    if (format == RTAUDIO_SINT24)
    {
        throw Napi::Error::New(info.Env(),
                               "24-bit signed integer is not available!");
    }

    // If there is already an error threadsafe-function, release it
    if (errorTsfn != nullptr)
    {
        errorTsfn.Release();
    }

    // If there is already an input threadsafe-function, release it
    if (_inputTsfn != nullptr)
    {
        _inputTsfn.Release();
    }

    // If there is already a frame output threadsafe-function, release it
    if (_frameOutputTsfn != nullptr)
    {
        _frameOutputTsfn.Release();
    }

    // Save input and output channels
    _inputChannels = inputParams.nChannels;
    _outputChannels = outputParams.nChannels;

    // Save the sample size by the format
    _sampleSize = getSampleSizeForFormat(format);

    // Save the format
    _format = format;

    // Set stream options
    options.flags = flags;
    options.streamName = streamName;

    // Create ThreadSafeFunction for error callback
    if (!errorCallback.IsEmpty())
    {
        errorTsfn = Napi::ThreadSafeFunction::New(
            info.Env(),
            errorCallback,
            "errorCallback",
            0,
            1,
            [](Napi::Env) { });
    }

    try
    {
        // Open the stream
        checkRtAudio(_rtAudio->openStream(
                         info[0].IsNull() || info[0].IsUndefined()
                             ? nullptr
                             : &outputParams,
                         info[1].IsNull() || info[1].IsUndefined()
                             ? nullptr
                             : &inputParams,
                         format,
                         sampleRate,
                         &frameSize,
                         rt_callback,
                         this,
                         &options),
                     info.Env());
    }
    catch (std::exception& ex)
    {
        throw Napi::Error::New(info.Env(), ex.what());
    }

    // Save frame size after openStream() has been called in case frameSize was overridden or 0 for default
    _frameSize = frameSize;

    // Lock the threadsafe functions mutex
    tsfnLock.lock();

    // If the input callback isn't null and the input info isn't null
    if (!inputCallback.IsEmpty() && !info[1].IsNull() && !info[1].IsUndefined())
    {
        // Save the input callback as a thread safe function
        _inputTsfn = Napi::ThreadSafeFunction::New(
            info.Env(),
            inputCallback,
            "inputCallback",
            0,
            1,
            [](Napi::Env) { });
    }

    // If the frame output callback isn't null and the output info isn't null
    if (!frameOutputCallback.IsEmpty() && !info[0].IsNull() &&
        !info[0].IsUndefined())
    {
        // Save the output callback as a thread safe function
        _frameOutputTsfn = Napi::ThreadSafeFunction::New(
            info.Env(),
            frameOutputCallback,
            "frameOutputCallback",
            0,
            1,
            [](Napi::Env) { });
    }
    return Napi::Number::New(info.Env(), _frameSize);
}

void RtAudioWrap::closeStream(const Napi::CallbackInfo&)
{
    _rtAudio->closeStream();
}

Napi::Value RtAudioWrap::isStreamOpen(const Napi::CallbackInfo& info)
{
    return Napi::Boolean::New(info.Env(), _rtAudio->isStreamOpen());
}

void RtAudioWrap::start(const Napi::CallbackInfo& info)
{
    try
    {
        // Start the stream
        checkRtAudio(_rtAudio->startStream(), info.Env());
    }
    catch (std::exception& ex)
    {
        throw Napi::Error::New(info.Env(), ex.what());
    }
}

void RtAudioWrap::stop(const Napi::CallbackInfo& info)
{
    try
    {
        // Stop the stream
        checkRtAudio(_rtAudio->stopStream(), info.Env());
    }
    catch (std::exception& ex)
    {
        throw Napi::Error::New(info.Env(), ex.what());
    }
}

Napi::Value RtAudioWrap::isStreamRunning(const Napi::CallbackInfo& info)
{
    return Napi::Boolean::New(info.Env(), _rtAudio->isStreamRunning());
}

void RtAudioWrap::write(const Napi::CallbackInfo& info)
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
    data = std::shared_ptr<int8_t>(
        new int8_t[_frameSize * _outputChannels * _sampleSize]);

    // Copy the data to the new buffer
    memcpy(data.get(), buf.Data(), _frameSize * _outputChannels * _sampleSize);

    // Lock the output data queue mutex
    lk.lock();

    // Push the new PCM data to the queue
    _outputData.push(data);

    // Unlock the output data queue mutex
    lk.unlock();
}

void RtAudioWrap::clearOutputQueue(const Napi::CallbackInfo& info)
{
    std::lock_guard lk(_outputDataMutex);

    // Clear the output queue
    std::queue<std::shared_ptr<int8_t>> empty;
    std::swap(_outputData, empty);
}

Napi::Value RtAudioWrap::getApi(const Napi::CallbackInfo& info)
{
    return Napi::String::New(
        info.Env(),
        _rtAudio->getApiDisplayName(_rtAudio->getCurrentApi()));
}

Napi::Value RtAudioWrap::getStreamLatency(const Napi::CallbackInfo& info)
{
    try
    {
        return Napi::Number::New(info.Env(), _rtAudio->getStreamLatency());
    }
    catch (std::exception& ex)
    {
        throw Napi::Error::New(info.Env(), ex.what());
    }
}

Napi::Value RtAudioWrap::getStreamSampleRate(const Napi::CallbackInfo& info)
{
    try
    {
        return Napi::Number::New(info.Env(), _rtAudio->getStreamSampleRate());
    }
    catch (std::exception& ex)
    {
        throw Napi::Error::New(info.Env(), ex.what());
    }
}

Napi::Value RtAudioWrap::getStreamTime(const Napi::CallbackInfo& info)
{
    try
    {
        return Napi::Number::New(info.Env(), _rtAudio->getStreamTime());
    }
    catch (std::exception& ex)
    {
        throw Napi::Error::New(info.Env(), ex.what());
    }
}

void RtAudioWrap::setStreamTime(const Napi::CallbackInfo& info,
                                const Napi::Value& value)
{
    double time = value.As<Napi::Number>();

    try
    {
        _rtAudio->setStreamTime(time);
    }
    catch (std::exception& ex)
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

double RtAudioWrap::getSignalMultiplierForVolume(double volume)
{
    // Explained here: https://stackoverflow.com/a/1165188
    return (pow(10, volume) - 1) / (10 - 1);
}

void RtAudioWrap::applyVolume(void* src, void* dst, unsigned int amount)
{
    // Copy and apply volume
    for (unsigned int i = 0; i < amount; i++)
    {
        switch (_format)
        {
        case RTAUDIO_SINT8:
            *(static_cast<int8_t*>(dst) + i) =
                    static_cast<int8_t>(*(static_cast<int8_t*>(src) + i) * _outputMultiplier);
            break;

        case RTAUDIO_SINT16:
            *(static_cast<int16_t*>(dst) + i) =
                    static_cast<int16_t>(*(static_cast<int16_t*>(src) + i) * _outputMultiplier);
            break;

        case RTAUDIO_SINT32:
            *(static_cast<int32_t*>(dst) + i) =
                    static_cast<int32_t>(*(static_cast<int32_t*>(src) + i) * _outputMultiplier);
            break;

        case RTAUDIO_FLOAT32:
            *(static_cast<float*>(dst) + i) = static_cast<float>(*(static_cast<float*>(src) + i) * _outputMultiplier);
            break;

        case RTAUDIO_FLOAT64:
            *(static_cast<double*>(dst) + i) = *(static_cast<double*>(src) + i) * _outputMultiplier;
            break;
        }
    }
}

void RtAudioWrap::setOutputVolume(const Napi::CallbackInfo& info,
                                  const Napi::Value& value)
{
    std::lock_guard lk(_outputDataMutex);

    double volume = (double)value.As<Napi::Number>();

    // Check for valid volume
    if (volume < 0 || volume > 1)
    {
        throw Napi::Error::New(info.Env(), "Invalid volume value!");
    }

    // Calculate signal multiplier for volume
    _outputMultiplier = getSignalMultiplierForVolume(volume);

    // Save volume
    _outputVolume = volume;
}

Napi::Value RtAudioWrap::getOutputVolume(const Napi::CallbackInfo& info)
{
    std::lock_guard lk(_outputDataMutex);

    return Napi::Number::New(info.Env(), _outputVolume);
}

void RtAudioWrap::setInputCallback(const Napi::CallbackInfo& info)
{
    Napi::Function inputCallback = info[0].IsNull() || info[0].IsUndefined()
                                       ? Napi::Function()
                                       : info[0].As<Napi::Function>();

    std::lock_guard lk(_tsfnMutex);

    // If the input callback isn't null
    if (!inputCallback.IsEmpty())
    {
        // Save the input callback as a thread safe function
        _inputTsfn = Napi::ThreadSafeFunction::New(
            info.Env(),
            inputCallback,
            "inputCallback",
            0,
            1,
            [](Napi::Env) { });
    }
    else
    {
        _inputTsfn = Napi::ThreadSafeFunction();
    }
}

void RtAudioWrap::setFrameOutputCallback(const Napi::CallbackInfo& info)
{
    Napi::Function frameOutputCallback = info[0].IsNull() || info[0].IsUndefined()
                                             ? Napi::Function()
                                             : info[0].As<Napi::Function>();

    std::lock_guard lk(_tsfnMutex);

    // If the frame output callback isn't null
    if (!frameOutputCallback.IsEmpty())
    {
        // Save the output callback as a thread safe function
        _frameOutputTsfn = Napi::ThreadSafeFunction::New(
            info.Env(),
            frameOutputCallback,
            "frameOutputCallback",
            0,
            1,
            [](Napi::Env) { });
    }
    else
    {
        _frameOutputTsfn = Napi::ThreadSafeFunction();
    }
}

void RtAudioWrap::checkRtAudio(const RtAudioErrorType error,
                               const Napi::Env env) const
{
    if (error == RTAUDIO_NO_ERROR || error == RTAUDIO_WARNING)
    {
        return;
    }

    std::stringstream ss;

    ss << "RtAudio Error: Code: ";
    ss << static_cast<uint32_t>(error);
    ss << ", Message: '";
    ss << _rtAudio->getErrorText();
    ss << "'";

    throw Napi::Error::New(env, ss.str());
}
