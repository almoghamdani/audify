#pragma once

#include <RtAudio.h>
#include <napi.h>

class RtAudioConverter
{
public:
    static Napi::Value ConvertDeviceInfo(const Napi::Env env,
                                         const RtAudio::DeviceInfo& dev);

    static RtAudio::StreamParameters ConvertStreamParameters(
        const Napi::Object& obj,
        const bool is_input,
        RtAudio& rt_audio);

private:
    template <typename T>
    static Napi::Array ArrayFromVector(Napi::Env env, std::vector<T> items)
    {
        Napi::Array array = Napi::Array::New(env, items.size());

        // For each element, convert it to Napi::Value and add it
        for (uint32_t i = 0; i < static_cast<uint32_t>(items.size()); i++)
        {
            array[i] = Napi::Value::From(env, items[i]);
        }

        return array;
    };
};
