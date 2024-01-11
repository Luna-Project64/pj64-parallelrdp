#pragma once

#include "volk.h"

typedef struct vulkan_filter_chain vulkan_filter_chain_t;

struct vulkan_filter_chain_texture
{
    VkImage image;
    VkImageView view;
    VkImageLayout layout;
    unsigned width;
    unsigned height;
    VkFormat format;
};

typedef enum glslang_filter_chain_scale
{
    GLSLANG_FILTER_CHAIN_SCALE_ORIGINAL,
    GLSLANG_FILTER_CHAIN_SCALE_SOURCE,
    GLSLANG_FILTER_CHAIN_SCALE_VIEWPORT,
    GLSLANG_FILTER_CHAIN_SCALE_ABSOLUTE
} glslang_filter_chain_scale;

typedef enum glslang_filter_chain_filter
{
    GLSLANG_FILTER_CHAIN_LINEAR = 0,
    GLSLANG_FILTER_CHAIN_NEAREST = 1,
    GLSLANG_FILTER_CHAIN_COUNT
} glslang_filter_chain_filter;

typedef enum glslang_filter_chain_address
{
    GLSLANG_FILTER_CHAIN_ADDRESS_REPEAT = 0,
    GLSLANG_FILTER_CHAIN_ADDRESS_MIRRORED_REPEAT = 1,
    GLSLANG_FILTER_CHAIN_ADDRESS_CLAMP_TO_EDGE = 2,
    GLSLANG_FILTER_CHAIN_ADDRESS_CLAMP_TO_BORDER = 3,
    GLSLANG_FILTER_CHAIN_ADDRESS_MIRROR_CLAMP_TO_EDGE = 4,
    GLSLANG_FILTER_CHAIN_ADDRESS_COUNT
} glslang_filter_chain_address;

struct vulkan_filter_chain_pass_info
{
    /* Maximum number of mip-levels to use. */
    unsigned max_levels;

    float scale_x;
    float scale_y;

    /* Ignored for the last pass, swapchain info
     * will be used instead. */
    VkFormat rt_format;
    /* For the last pass, make sure VIEWPORT scale
     * with scale factors of 1 are used. */
    enum glslang_filter_chain_scale scale_type_x;
    enum glslang_filter_chain_scale scale_type_y;
    /* The filter to use for source in this pass. */
    enum glslang_filter_chain_filter source_filter;
    enum glslang_filter_chain_filter mip_filter;
    enum glslang_filter_chain_address address;
};

struct vulkan_filter_chain_swapchain_info
{
    VkViewport viewport;
    VkFormat format;
    VkRenderPass render_pass;
    unsigned num_indices;
};

struct vulkan_filter_chain_create_info
{
    VkDevice device;
    VkPhysicalDevice gpu;
    const VkPhysicalDeviceMemoryProperties* memory_properties;
    VkPipelineCache pipeline_cache;
    VkQueue queue;
    VkCommandPool command_pool;
    unsigned num_passes;

    VkFormat original_format;
    struct
    {
        unsigned width, height;
    } max_input_size;
    struct vulkan_filter_chain_swapchain_info swapchain;
};

#ifdef __cplusplus
extern "C" {
#endif

    vulkan_filter_chain_t* vulkan_filter_chain_create_default(
        const struct vulkan_filter_chain_create_info* info,
        enum glslang_filter_chain_filter filter);
    void vulkan_filter_chain_free(vulkan_filter_chain_t* chain);
    bool vulkan_filter_chain_update_swapchain_info(vulkan_filter_chain_t* chain,
        const struct vulkan_filter_chain_swapchain_info* info);
    void vulkan_filter_chain_notify_sync_index(vulkan_filter_chain_t* chain,
        unsigned index);
    void vulkan_filter_chain_set_frame_count(
        vulkan_filter_chain_t* chain,
        uint64_t count);
    void vulkan_filter_chain_set_frame_direction(
        vulkan_filter_chain_t* chain,
        int32_t direction);
    void vulkan_filter_chain_set_input_texture(
        vulkan_filter_chain_t* chain,
        const struct vulkan_filter_chain_texture* texture);
    void vulkan_filter_chain_build_offscreen_passes(
        vulkan_filter_chain_t* chain,
        VkCommandBuffer cmd, const VkViewport* vp);
    void vulkan_filter_chain_build_viewport_pass(
        vulkan_filter_chain_t* chain,
        VkCommandBuffer cmd, const VkViewport* vp, const float* mvp);
    void vulkan_filter_chain_end_frame(
        vulkan_filter_chain_t* chain,
        VkCommandBuffer cmd);
    struct video_shader* vulkan_filter_chain_get_preset(
        vulkan_filter_chain_t* chain);

#ifdef __cplusplus
}
#endif
