#include "gfx_display.h"

static gfx_display_t dispgfx_st = { 0 };

gfx_display_t* disp_get_ptr(void)
{
	return &dispgfx_st;
}

bool gfx_display_init_first_driver(gfx_display_t* p_disp,
    bool video_is_threaded)
{
    p_disp->dispctx = &gfx_display_ctx_vulkan;
    return true;
}
