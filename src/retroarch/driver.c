#include "driver.h"

#include "video_driver.h"
#include "gfx_display.h"

void drivers_init(
    settings_t* settings,
    int flags,
    bool verbosity_enabled)
{
    video_driver_state_t
        * video_st = video_state_get_ptr();
    bool video_is_threaded = false;
    gfx_display_t* p_disp = disp_get_ptr();

    /* Initialize video driver */
    {
        struct retro_hw_render_callback* hwr =
            VIDEO_DRIVER_GET_HW_CONTEXT_INTERNAL(video_st);

        video_driver_lock_new();
        video_driver_set_cached_frame_ptr(NULL);
        if (!video_driver_init_internal(&video_is_threaded,
            verbosity_enabled))
            retroarch_fail(1, "video_driver_init_internal()");

        if (!video_st->cache_context_ack
            && hwr->context_reset)
            hwr->context_reset();
    }

#if 0
    core_info_init_current_core();
#endif

    gfx_display_init_first_driver(p_disp, video_is_threaded);
}

void driver_uninit(int flags)
{
    video_driver_state_t
        * video_st = video_state_get_ptr();

    video_driver_free_internal();
    VIDEO_DRIVER_LOCK_FREE(video_st);
    video_st->data = NULL;
    video_driver_set_cached_frame_ptr(NULL);

    video_st->data = NULL;
}
