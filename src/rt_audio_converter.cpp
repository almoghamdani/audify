#include "rt_audio_converter.h"

Napi::Value RtAudioConverter::ConvertDeviceInfo(
    const Napi::Env env,
    const RtAudio::DeviceInfo& dev)
{
    Napi::Object devInfo = Napi::Object::New(env);

    // Set all properties in the object
    devInfo.Set("id", dev.ID);
    devInfo.Set("name", dev.name);
    devInfo.Set("outputChannels", dev.outputChannels);
    devInfo.Set("inputChannels", dev.inputChannels);
    devInfo.Set("duplexChannels", dev.duplexChannels);
    devInfo.Set("isDefaultOutput", dev.isDefaultOutput);
    devInfo.Set("isDefaultInput", dev.isDefaultInput);
    devInfo.Set("sampleRates", ArrayFromVector(env, dev.sampleRates));
    devInfo.Set("preferredSampleRate", dev.preferredSampleRate);
    devInfo.Set("nativeFormats", dev.nativeFormats);

    return devInfo;
}

RtAudio::StreamParameters RtAudioConverter::ConvertStreamParameters(
    const Napi::Object& obj,
    const bool is_input,
    RtAudio& rt_audio)
{
    RtAudio::StreamParameters params;

    if (!obj.Has("deviceId"))
    {
        // Set default device id
        params.deviceId = is_input
                              ? rt_audio.getDefaultInputDevice()
                              : rt_audio.getDefaultOutputDevice();
    }
    else if (!obj.Get("deviceId").IsNumber())
    {
        throw Napi::Error::New(obj.Env(),
                               "The 'device id' stream parameter is missing or invalid!");
    }
    else
    {
        params.deviceId = obj.Get("deviceId").As<Napi::Number>();

        // Legacy: Older users of RtAudio/Audify were required to input the device index and not the device's unique ID.
        // In RtAudio 6.0.0 a unique device id is now exposed in the list of devices and should be used instead.
        // The unique device id minimum value is 129, so we use it to differentiate between old and new users.
        static const uint32_t MIN_UNIQUE_DEVICE_ID = 129;
        if (params.deviceId < MIN_UNIQUE_DEVICE_ID)
        {
            const auto devices = rt_audio.getDeviceIds();
            if (params.deviceId >= devices.size())
            {
                throw Napi::Error::New(obj.Env(),
                                       "The 'device id' stream parameter is invalid!");
            }

            params.deviceId = devices[params.deviceId];
        }
    }

    if (!obj.Has("firstChannel"))
    {
        // Set default first channel
        params.firstChannel = 0;
    }
    else if (!obj.Get("firstChannel").IsNumber())
    {
        throw Napi::Error::New(
            obj.Env(),
            "The 'first channel' stream parameter is missing or invalid!");
    }
    else
    {
        params.firstChannel = obj.Get("firstChannel").As<Napi::Number>();
    }

    if (!obj.Has("nChannels") || !obj.Get("nChannels").IsNumber())
    {
        throw Napi::Error::New(
            obj.Env(),
            "The 'no. of channels' stream parameter is missing or invalid!");
    }

    params.nChannels = obj.Get("nChannels").As<Napi::Number>();

    return params;
}
