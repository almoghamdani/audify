#pragma once

#include <napi.h>
#include <opus.h>

class OpusDecoderWrap : public Napi::ObjectWrap<OpusDecoderWrap>
{
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    static void Destroy();

public:
    OpusDecoderWrap(const Napi::CallbackInfo& info);
    ~OpusDecoderWrap() override;

    Napi::Value decode(const Napi::CallbackInfo& info);
    Napi::Value decodeFloat(const Napi::CallbackInfo& info);

private:
    inline static Napi::FunctionReference constructor;

    std::string getErrorMsg(int error);

    OpusDecoder* _opusDecoder;
    int _opus_error;
    int _channels;
};
