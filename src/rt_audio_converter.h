#pragma once

#include <napi.h>
#include <RtAudio.h>

class RtAudioConverter
{
public:
	static Napi::Value ConvertDeviceInfo(Napi::Env env, RtAudio::DeviceInfo& dev);
	
	static RtAudio::StreamParameters ConvertStreamParameters(Napi::Object obj);

private:
	template <typename T>
	static Napi::Array ArrayFromVector(Napi::Env env, std::vector<T> items)
	{
		Napi::Array array = Napi::Array::New(env, items.size());

		// For each element, convert it to Napi::Value and add it
		for (int i = 0; i < items.size(); i++)
		{
			array[i] = Napi::Value::From(env, items[i]);
		}

		return array;
	};
};