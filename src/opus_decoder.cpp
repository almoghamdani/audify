#include "opus_decoder.h"

Napi::Object OpusDecoderWrap::Init(Napi::Env env, Napi::Object exports)
{
    // Define the class and get it's ctor function
    Napi::Function ctor_func = DefineClass(
        env,
        "OpusDecoder",
        {
            InstanceMethod("decode", &OpusDecoderWrap::decode),
            InstanceMethod("decodeFloat", &OpusDecoderWrap::decodeFloat)
        });

    // Set the class's ctor function as a persistent object to keep it in memory
    constructor = Napi::Persistent(ctor_func);
    constructor.SuppressDestruct();

    // Export the ctor
    exports.Set("OpusDecoder", ctor_func);
    return exports;
}

void OpusDecoderWrap::Destroy()
{
    constructor.Reset();
}

OpusDecoderWrap::OpusDecoderWrap(const Napi::CallbackInfo& info) :
    Napi::ObjectWrap<OpusDecoderWrap>(info),
    _opusDecoder(nullptr),
    _channels(0)
{
    int error = 0;

    int sampleRate = info[0].As<Napi::Number>();
    _channels = info[1].As<Napi::Number>();

    // Init Opus Decoder
    _opusDecoder = opus_decoder_create(sampleRate, _channels, &error);
    if (error != OPUS_OK)
    {
        throw Napi::Error::New(info.Env(), getErrorMsg(error));
    }
}

OpusDecoderWrap::~OpusDecoderWrap() { opus_decoder_destroy(_opusDecoder); }

Napi::Value OpusDecoderWrap::decode(const Napi::CallbackInfo& info)
{
    Napi::Buffer buf = info[0].As<Napi::Buffer<uint8_t>>();
    int frameSize = info[1].As<Napi::Number>();

    opus_int16* outBuf = new opus_int16[frameSize * _channels];
    int samples = 0;

    // Decode the given frame
    samples = opus_decode(_opusDecoder,
                          (const unsigned char*)buf.Data(),
                          (opus_int32)buf.Length(),
                          outBuf,
                          frameSize,
                          0);
    if (samples < 0)
    {
        throw Napi::Error::New(info.Env(), getErrorMsg(samples));
    }

    // Create Napi Buffer from output buffer
    return Napi::Buffer<uint8_t>::Copy(info.Env(),
                                       (const uint8_t*)outBuf,
                                       samples * _channels * sizeof(opus_int16));
}

Napi::Value OpusDecoderWrap::decodeFloat(const Napi::CallbackInfo& info)
{
    Napi::Buffer buf = info[0].As<Napi::Buffer<uint8_t>>();
    int frameSize = info[1].As<Napi::Number>();

    float* outBuf = new float[frameSize * _channels];
    int samples = 0;

    // Decode the given frame
    samples = opus_decode_float(_opusDecoder,
                                (const unsigned char*)buf.Data(),
                                static_cast<opus_int32>(buf.Length()),
                                outBuf,
                                frameSize,
                                0);
    if (samples < 0)
    {
        throw Napi::Error::New(info.Env(), getErrorMsg(samples));
    }

    // Create Napi Buffer from output buffer
    return Napi::Buffer<uint8_t>::Copy(info.Env(),
                                       (const uint8_t*)outBuf,
                                       samples * _channels * sizeof(float));
}

std::string OpusDecoderWrap::getErrorMsg(int error)
{
    switch (error)
    {
    case OPUS_BAD_ARG:
        return "One or more invalid/out of range arguments!";
    case OPUS_BUFFER_TOO_SMALL:
        return "The mode struct passed is invalid!";
    case OPUS_INTERNAL_ERROR:
        return "An internal error was detected!";
    case OPUS_INVALID_PACKET:
        return "The compressed data passed is corrupted!";
    case OPUS_UNIMPLEMENTED:
        return "Invalid/unsupported request number!";
    case OPUS_INVALID_STATE:
        return "An decoder or decoder structure is invalid or already freed!";
    case OPUS_ALLOC_FAIL:
        return "Memory allocation has failed!";
    default:
        return "Unknown Opus error!";
    }
}
