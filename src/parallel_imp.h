#ifndef PARALLEL_RDP_HPP
#define PARALLEL_RDP_HPP

#include "rdp_device.hpp"
#include "context.hpp"
#include "device.hpp"
#include "retroarch/retroarch.h"

namespace RDP
{
bool init();
void deinit();
void begin_frame();

void process_commands();
extern const struct retro_hw_render_interface_vulkan *vulkan;

extern unsigned width;
extern unsigned height;
extern unsigned upscaling;
extern unsigned overscan;
extern unsigned downscaling_steps;
extern bool synchronous, divot_filter, gamma_dither, vi_aa, vi_scale, dither_filter, interlacing;
extern bool native_texture_lod, native_tex_rect, super_sampled_read_back, super_sampled_dither;

void complete_frame();
void deinit();

void profile_refresh_begin();
void profile_refresh_end();
}

#ifdef __cplusplus
extern "C" {
#endif

bool parallel_retro_init_vulkan(void);

#ifdef __cplusplus
}
#endif

#endif