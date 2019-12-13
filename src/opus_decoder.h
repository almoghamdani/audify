#pragma once

#include <napi.h>
#include <opus.h>

class OpusDecoderWrap : public Napi::ObjectWrap<OpusDecoderWrap>
{
public:
	static Napi::Object Init(Napi::Env env, Napi::Object exports);
	OpusDecoderWrap(const Napi::CallbackInfo &info);
	~OpusDecoderWrap();

	Napi::Value decode(const Napi::CallbackInfo &info);
	Napi::Value decodeFloat(const Napi::CallbackInfo &info);

	void setBitrate(const Napi::CallbackInfo &info, const Napi::Value &value);
	Napi::Value getBitrate(const Napi::CallbackInfo &info);

private:
	inline static Napi::FunctionReference constructor;

	std::string getErrorMsg(int error);

	OpusDecoder *_opusDecoder;
	int _opus_error;
	int _channels;
};