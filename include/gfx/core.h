#ifndef __GFX_CORE_H
#define __GFX_CORE_H

#include <vbe.h>

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

struct graphic_context* gfx_new_ctx();
int gfx_init_framebuffer(struct graphic_context* ctx, struct vbe_mode_info_structure* vbe_info);

#endif /* __GFX_CORE_H */
