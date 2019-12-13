#include "rt_audio_converter.h"

Napi::Value RtAudioConverter::ConvertDeviceInfo(Napi::Env env, RtAudio::DeviceInfo &dev)
{
	Napi::Object devInfo = Napi::Object::New(env);

	// Set all properties in the object
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

RtAudio::StreamParameters RtAudioConverter::ConvertStreamParameters(Napi::Object obj)
{
	RtAudio::StreamParameters params;

	// Check for valid device id
	if (!obj.Has("deviceId") || !obj.Get("deviceId").IsNumber())
	{
		throw Napi::Error::New(obj.Env(), "The 'device id' stream parameter is missing or invalid!");
	}

	// Check for valid no. of channels
	if (!obj.Has("nChannels") || !obj.Get("nChannels").IsNumber())
	{
		throw Napi::Error::New(obj.Env(), "The 'no. of channels' stream parameter is missing or invalid!");
	}

	// Check for valid first channel
	if (!obj.Has("firstChannel") || !obj.Get("firstChannel").IsNumber())
	{
		throw Napi::Error::New(obj.Env(), "The 'first channel' stream parameter is missing or invalid!");
	}

	// Get the properties from the object
	params.deviceId = obj.Get("deviceId").As<Napi::Number>();
	params.nChannels = obj.Get("nChannels").As<Napi::Number>();
	params.firstChannel = obj.Get("firstChannel").As<Napi::Number>();

	return params;
}