#include "opus_encoder.h"

Napi::Object OpusEncoderWrap::Init(Napi::Env env, Napi::Object exports)
{
    // Define the class and get it's ctor function
    Napi::Function ctor_func =
            DefineClass(env,
                        "OpusEncoder",
                        {
                            InstanceMethod("encode", &OpusEncoderWrap::encode),
                            InstanceMethod("encodeFloat", &OpusEncoderWrap::encodeFloat),
                            InstanceAccessor("bitrate",
                                             &OpusEncoderWrap::getBitrate,
                                             &OpusEncoderWrap::setBitrate)
                        });

    // Set the class's ctor function as a persistent object to keep it in memory
    constructor = Napi::Persistent(ctor_func);
    constructor.SuppressDestruct();

    // Export the ctor
    exports.Set("OpusEncoder", ctor_func);
    return exports;
}

void OpusEncoderWrap::Destroy()
{
    constructor.Reset();
}

OpusEncoderWrap::OpusEncoderWrap(const Napi::CallbackInfo& info) :
    Napi::ObjectWrap<OpusEncoderWrap>(info),
    _opusEncoder(nullptr),
    _channels(0)
{
    int error = 0;

    int sampleRate = info[0].As<Napi::Number>(),
        application = info[2].As<Napi::Number>();
    _channels = info[1].As<Napi::Number>();

    // Init Opus Encoder
    _opusEncoder =
            opus_encoder_create(sampleRate, _channels, application, &error);
    if (error != OPUS_OK)
    {
        throw Napi::Error::New(info.Env(), getErrorMsg(error));
    }
}

OpusEncoderWrap::~OpusEncoderWrap()
{
    opus_encoder_destroy(_opusEncoder);
}

Napi::Value OpusEncoderWrap::encode(const Napi::CallbackInfo& info)
{
    Napi::Buffer buf = info[0].As<Napi::Buffer<uint8_t>>();
    int frameSize = info[1].As<Napi::Number>();

    unsigned char outBuf[MAX_DATA_SIZE] = {0};
    int outBufSize = 0;

    // Check for valid frame buffer length
    if (buf.Length() != frameSize * _channels * sizeof(opus_int16))
    {
        throw Napi::Error::New(info.Env(),
                               "Frame buffer length should be "
                               "(frame_size*channels*sizeof(opus_int16))!");
    }

    // Encode the given frame
    outBufSize = opus_encode(_opusEncoder,
                             (opus_int16*)buf.Data(),
                             frameSize,
                             outBuf,
                             MAX_DATA_SIZE);
    if (outBufSize < 0)
    {
        throw Napi::Error::New(info.Env(), getErrorMsg(outBufSize));
    }

    // Create Napi Buffer from output buffer
    return Napi::Buffer<uint8_t>::Copy(info.Env(), outBuf, outBufSize);
}

Napi::Value OpusEncoderWrap::encodeFloat(const Napi::CallbackInfo& info)
{
    Napi::Buffer buf = info[0].As<Napi::Buffer<uint8_t>>();
    int frameSize = info[1].As<Napi::Number>();

    unsigned char outBuf[MAX_DATA_SIZE] = {0};
    int outBufSize = 0;

    // Check for valid frame buffer length
    if (buf.Length() != frameSize * _channels * sizeof(float))
    {
        throw Napi::Error::New(info.Env(),
                               "Frame buffer length should be "
                               "(frame_size*channels*sizeof(opus_int16))!");
    }

    // Encode the given frame
    outBufSize = opus_encode_float(_opusEncoder,
                                   (float*)buf.Data(),
                                   frameSize,
                                   outBuf,
                                   MAX_DATA_SIZE);
    if (outBufSize < 0)
    {
        throw Napi::Error::New(info.Env(), getErrorMsg(outBufSize));
    }

    // Create Napi Buffer from output buffer
    return Napi::Buffer<uint8_t>::Copy(info.Env(), outBuf, outBufSize);
}

void OpusEncoderWrap::setBitrate(const Napi::CallbackInfo& info,
                                 const Napi::Value& value)
{
    unsigned int bitrate = value.As<Napi::Number>();

    // Set encoder's bitrate
    int error = opus_encoder_ctl(_opusEncoder, OPUS_SET_BITRATE(bitrate));
    if (error != OPUS_OK)
    {
        throw Napi::Error::New(info.Env(), "Invalid bitrate!");
    }
}

Napi::Value OpusEncoderWrap::getBitrate(const Napi::CallbackInfo& info)
{
    opus_int32 bitrate = 0;

    // Set encoder's bitrate
    int error = opus_encoder_ctl(_opusEncoder, OPUS_GET_BITRATE(&bitrate));
    if (error != OPUS_OK)
    {
        throw Napi::Error::New(info.Env(), getErrorMsg(error));
    }

    return Napi::Number::New(info.Env(), bitrate);
}

std::string OpusEncoderWrap::getErrorMsg(int error)
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
        return "An encoder or decoder structure is invalid or already freed!";
    case OPUS_ALLOC_FAIL:
        return "Memory allocation has failed!";
    default:
        return "Unknown Opus error!";
    }
}
