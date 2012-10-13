#ifndef MTAP_TILE_CALLBACK_H
#define MTAP_TILE_CALLBACK_H

#include "renderer/api/rendering.h"
#include "foundation/image/tile.h"

namespace asf = foundation;
namespace asr = renderer;

class mtap_ITileCallback : public asr::ITileCallback
{
public:

    // Delete this instance.
    virtual void release(){ delete this;};

   // This method is called before a region is rendered.
    void pre_render(
        const size_t x,
        const size_t y,
        const size_t width,
        const size_t height);

    // This method is called after a whole frame is rendered (at once).
    void post_render(
        const asr::Frame& frame);

    // This method is called after a tile is rendered.
    void post_render(
        const asr::Frame& frame,
        const size_t tile_x,
        const size_t tile_y);
};

class mtap_ITileCallbackFactory : public asr::ITileCallbackFactory
{
public:
	virtual asr::ITileCallback* create();
	virtual void release(){delete this;};
};

#endif