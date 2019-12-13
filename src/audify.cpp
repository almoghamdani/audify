#include <napi.h>

#include "opus_encoder.h"
#include "opus_decoder.h"

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
	OpusEncoderWrap::Init(env, exports);
	OpusDecoderWrap::Init(env, exports);

    return exports;
}

NODE_API_MODULE(audify, Init)