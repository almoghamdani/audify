#pragma once

#include <RtAudio.h>
#include <napi.h>

#include <memory>
#include <mutex>
#include <queue>

class RtAudioWrap : public Napi::ObjectWrap<RtAudioWrap>
{
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    static void Destroy();

public:
    RtAudioWrap(const Napi::CallbackInfo& info);
    ~RtAudioWrap() override;

    Napi::Value getDevices(const Napi::CallbackInfo& info);
    Napi::Value getDefaultInputDevice(const Napi::CallbackInfo& info);
    Napi::Value getDefaultOutputDevice(const Napi::CallbackInfo& info);

    Napi::Value openStream(const Napi::CallbackInfo& info);
    void closeStream(const Napi::CallbackInfo& info);
    Napi::Value isStreamOpen(const Napi::CallbackInfo& info);

    void start(const Napi::CallbackInfo& info);
    void stop(const Napi::CallbackInfo& info);
    Napi::Value isStreamRunning(const Napi::CallbackInfo& info);

    void write(const Napi::CallbackInfo& info);
    void clearOutputQueue(const Napi::CallbackInfo& info);

    Napi::Value getApi(const Napi::CallbackInfo& info);
    Napi::Value getStreamLatency(const Napi::CallbackInfo& info);
    Napi::Value getStreamSampleRate(const Napi::CallbackInfo& info);

    Napi::Value getStreamTime(const Napi::CallbackInfo& info);
    void setStreamTime(const Napi::CallbackInfo& info, const Napi::Value& value);

    void setOutputVolume(const Napi::CallbackInfo& info,
                         const Napi::Value& value);
    Napi::Value getOutputVolume(const Napi::CallbackInfo& info);

    void setInputCallback(const Napi::CallbackInfo& info);
    void setFrameOutputCallback(const Napi::CallbackInfo& info);

private:
    unsigned int getSampleSizeForFormat(RtAudioFormat format);
    double getSignalMultiplierForVolume(double volume);
    void applyVolume(void* src, void* dst, unsigned int amount);

    void checkRtAudio(const RtAudioErrorType error, const Napi::Env env) const;

private:
    friend int rt_callback(void* outputBuffer,
                           void* inputBuffer,
                           unsigned int nFrames,
                           double streamTime,
                           RtAudioStreamStatus status,
                           void* userData);

private:
    inline static Napi::FunctionReference constructor;

    std::shared_ptr<RtAudio> _rtAudio;
    unsigned int _frameSize;
    unsigned int _inputChannels;
    unsigned int _outputChannels;
    unsigned int _sampleSize;
    RtAudioFormat _format;

    std::mutex _tsfnMutex;
    Napi::ThreadSafeFunction _inputTsfn;
    Napi::ThreadSafeFunction _frameOutputTsfn;

    std::mutex _outputDataMutex;
    std::queue<std::shared_ptr<int8_t>> _outputData;
    double _outputVolume;
    double _outputMultiplier;
};
