#pragma once

#include <napi.h>
#include <opus.h>

#define MAX_DATA_SIZE 2000

class OpusEncoderWrap : public Napi::ObjectWrap<OpusEncoderWrap>
{
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    static void Destroy();

public:
    OpusEncoderWrap(const Napi::CallbackInfo& info);
    ~OpusEncoderWrap() override;

    Napi::Value encode(const Napi::CallbackInfo& info);
    Napi::Value encodeFloat(const Napi::CallbackInfo& info);

    void setBitrate(const Napi::CallbackInfo& info, const Napi::Value& value);
    Napi::Value getBitrate(const Napi::CallbackInfo& info);

private:
    inline static Napi::FunctionReference constructor;

    std::string getErrorMsg(int error);

    OpusEncoder* _opusEncoder;
    int _opus_error;
    int _channels;
};
