#ifndef __GFX_CORE_H
#define __GFX_CORE_H

typedef enum __gfx_ctxs {
    GFX_CTX_NONE,
    GFX_CTX_DEFAULT,
} gfx_ctx_t;

struct graphic_context {
    gfx_ctx_t ctx;

    int width;
    int height;
    int bpp;
    int pitch;
    void *framebuffer;
};

#endif /* __GFX_CORE_H */
