#pragma once

#include "Common.h"

namespace kraken{

enum KrakenRenderBufferFormat{
    EFloat4 = 0,
    EFloat3,
    EFloat,
};

struct KrakenRenderBuffer
{
    Vec2i resolution;
    KrakenRenderBufferFormat format;
    void* data;

    void resize(Vec2i& size);
    void clear();
};

struct KrakenAccumulatedRenderBuffer: public KrakenRenderBuffer {
    uint32_t* samples;
};

}