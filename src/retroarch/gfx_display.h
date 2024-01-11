/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2017 - Daniel De Matteis
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

enum gfx_display_prim_type
{
	GFX_DISPLAY_PRIM_NONE = 0,
	GFX_DISPLAY_PRIM_TRIANGLESTRIP,
	GFX_DISPLAY_PRIM_TRIANGLES
};

typedef struct gfx_display_ctx_draw gfx_display_ctx_draw_t;
typedef struct gfx_display gfx_display_t;

typedef struct gfx_display_ctx_driver
{
	/* Draw graphics to the screen. */
	void (*draw)(gfx_display_ctx_draw_t* draw,
		void* data, unsigned video_width, unsigned video_height);
	/* Draw one of the menu pipeline shaders. */
	void (*draw_pipeline)(gfx_display_ctx_draw_t* draw,
		gfx_display_t* p_disp,
		void* data, unsigned video_width, unsigned video_height);
	/* Start blending operation. */
	void (*blend_begin)(void* data);
	/* Finish blending operation. */
	void (*blend_end)(void* data);
	/* Get the default Model-View-Projection matrix */
	void* (*get_default_mvp)(void* data);
	/* Get the default vertices matrix */
	const float* (*get_default_vertices)(void);
	/* Get the default texture coordinates matrix */
	const float* (*get_default_tex_coords)(void);
	/* Initialize the first compatible font driver for this menu driver. */
	bool (*font_init_first)(
		void** font_handle, void* video_data,
		const char* font_path, float font_size,
		bool is_threaded);
	// type == GFX_VIDEO_DRIVER_VULKAN
	const char* ident;
	bool handles_transform;
	/* Enables and disables scissoring */
	void (*scissor_begin)(void* data, unsigned video_width,
		unsigned video_height,
		int x, int y, unsigned width, unsigned height);
	void (*scissor_end)(void* data, unsigned video_width,
		unsigned video_height);
} gfx_display_ctx_driver_t;

typedef struct video_coords
{
	const float* vertex;
	const float* color;
	const float* tex_coord;
	const float* lut_tex_coord;
	const unsigned* index;
	unsigned vertices;
	unsigned indexes;
} video_coords_t;

struct gfx_display_ctx_draw
{
	float* color;
	const float* vertex;
	const float* tex_coord;
	const void* backend_data;
	struct video_coords* coords;
	void* matrix_data;
	uintptr_t texture;
	size_t vertex_count;
	size_t backend_data_size;
	unsigned width;
	unsigned height;
	unsigned pipeline_id;
	float x;
	float y;
	float rotation;
	float scale_factor;
	enum gfx_display_prim_type prim_type;
	bool pipeline_active;
};

struct gfx_display
{
	gfx_display_ctx_driver_t* dispctx;

	/* Width, height and pitch of the display framebuffer */
	size_t   framebuf_pitch;
	unsigned framebuf_width;
	unsigned framebuf_height;

	/* Height of the display header */
	unsigned header_height;

	bool has_windowed;
	bool msg_force;
	bool framebuf_dirty;
};

typedef struct gfx_display gfx_display_t;

gfx_display_t* disp_get_ptr(void);

bool gfx_display_init_first_driver(gfx_display_t* p_disp,
	bool video_is_threaded);

extern gfx_display_ctx_driver_t gfx_display_ctx_vulkan;
