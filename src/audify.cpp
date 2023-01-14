#include <napi.h>

#include "opus_decoder.h"
#include "opus_encoder.h"
#include "rt_audio.h"

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    OpusEncoderWrap::Init(env, exports);
    OpusDecoderWrap::Init(env, exports);
    RtAudioWrap::Init(env, exports);

    const napi_status add_cleanup_hook_status = napi_add_env_cleanup_hook(
        env,
        [](void*)
        {
            OpusEncoderWrap::Destroy();
            OpusDecoderWrap::Destroy();
            RtAudioWrap::Destroy();
        },
        nullptr);
    NAPI_THROW_IF_FAILED_VOID(env, add_cleanup_hook_status);

    return exports;
}

NODE_API_MODULE(audify, Init)
