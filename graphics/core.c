#include <kutils.h>
#include <gfx/core.h>
#include <memory.h>
#include <errors.h>
#include <colors.h>

struct graphic_context* gfx_new_ctx()
{
    struct graphic_context* ctx = create(struct graphic_context);
    ERR_ON_NULL_PTR(ctx);

    ctx->width = 0;
    ctx->height = 0;
    ctx->bpp = 0;
    ctx->pitch = 0;
    return ctx;
}

int gfx_init_framebuffer(struct graphic_context* ctx, struct vbe_mode_info_structure* vbe_info)
{
    ERR_ON_NULL(ctx);
    ERR_ON_NULL(vbe_info);

    ctx->width = vbe_info->width;
    ctx->height = vbe_info->height;
    ctx->bpp = vbe_info->bpp;
    ctx->pitch = vbe_info->pitch;
    ctx->framebuffer = (void*)vbe_info->framebuffer;


    dbgprintf("[VBE] INFO:\n");
	dbgprintf("[VBE] Height: %d\n", vbe_info->height);
	dbgprintf("[VBE] Width: %d\n", vbe_info->width);
	dbgprintf("[VBE] Pitch: %d\n", vbe_info->pitch);
	dbgprintf("[VBE] Bpp: %d\n", vbe_info->bpp);
	dbgprintf("[VBE] Memory Size: %d (0x%x)\n", vbe_info->width*vbe_info->height*(vbe_info->bpp/8), vbe_info->width*vbe_info->height*(vbe_info->bpp/8));

    /* memory map */
    vmem_map_driver_region(vbe_info->framebuffer, (((vbe_info->width*vbe_info->height*(vbe_info->bpp/8))+1)/PAGE_SIZE)+1);

    rgb_init_color_table();

    return 0;
}

