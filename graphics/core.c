#include <kutils.h>
#include <gfx/core.h>
#include <memory.h>
#include <errors.h>

struct graphic_context* gfx_new_ctx()
{
    struct graphic_context* ctx = create(struct graphic_context);
    ERR_ON_NULL(ctx);

    ctx->width = 0;
    ctx->height = 0;
    ctx->bpp = 0;
    ctx->pitch = 0;
    return ctx;
}


