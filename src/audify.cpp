#include <napi.h>

#include "opus_encoder.h"

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
	OpusEncoderWrap::Init(env, exports);

    return exports;
}

NODE_API_MODULE(audify, Init)