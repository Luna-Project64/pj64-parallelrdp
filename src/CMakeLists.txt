# Compiler flags mostly copied from parallel-rdp upstream CMakeLists.txt.
# MSVC: Added /O2, /fp:fast for significant performance increase, /MT for static linking against runtime library.
if (CMAKE_COMPILER_IS_GNUCXX OR (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang"))
    set(PJ64_PARALLEL_RDP_CXX_FLAGS -Wall -Wextra -Wno-missing-field-initializers -Wno-empty-body -ffast-math -Wno-unused-parameter -pthread -fwrapv)
elseif (MSVC)
    set(PJ64_PARALLEL_RDP_CXX_FLAGS /D_CRT_SECURE_NO_WARNINGS /wd4267 /wd4244 /wd4309 /wd4005 /MP /DNOMINMAX)
endif()

# Include CMakeLists.txt for parallel-rdp-standalone source.
include(${CMAKE_CURRENT_SOURCE_DIR}/parallel-rdp-standalone.cmake)

set(src_files
    parallel_imp.cpp
    config_gui.c
    config.c
    gfx_1.3.cpp
    ini.c
    retroarch/vulkan_common.c
    retroarch/w_vk_ctx.c
    retroarch/retro_vulkan.c
    retroarch/video_driver.c
    retroarch/driver.c
    retroarch/retroarch.c
    retroarch/win32_common.c
    retroarch/gfx_display_vulkan.c
    retroarch/gfx_display.c
    retroarch/dispserv_win32.c
    retroarch/rthreads.c
    retroarch/scaler.c
    retroarch/pixconv.c
    retroarch/scaler_filter.c
    retroarch/scaler_int.c
    retroarch/shader_vulkan.cpp
    retroarch/compat_strl.c
    retroarch/string_list.c
    retroarch/slang_reflection.cpp
    spirv-cross/spirv_cfg.cpp
    spirv-cross/spirv_cross.cpp
    spirv-cross/spirv_cross_parsed_ir.cpp
    spirv-cross/spirv_parser.cpp
)

set(header_files
    config_gui_resources.h
    config_gui.h
    config.h
    gfx_1.3.h
    gfx_m64p.h
    gfxstructdefs.h
    ini.h
    parallel_imp.h
    retroarch/vulkan_common.h
    retroarch/video_driver.h
    retroarch/driver.h
    retroarch/retroarch.h
    retroarch/win32_common.h
    retroarch/gfx_display.h
    retroarch/dispserv_win32.h
    retroarch/rthreads.h
    retroarch/scaler.h
    retroarch/filter.h
    retroarch/pixconv.h
    retroarch/scaler_int.h
    retroarch/shader_vulkan.h
    retroarch/compat_strl.h
    retroarch/string_list.h
    retroarch/slang_reflection.h
    spirv-cross/GLSL.std.450.h
    spirv-cross/spirv.h
    spirv-cross/spirv_cfg.hpp
    spirv-cross/spirv_common.hpp
    spirv-cross/spirv_cross.hpp
    spirv-cross/spirv_cross_parsed_ir.hpp
    spirv-cross/spirv_parser.hpp
    spirv-cross/spirv_cross_containers.hpp
    spirv-cross/spirv_cross_error_handling.hpp
)

set(res_files
    config_gui_resources.rc
)

# Windows *.lib file dependencies.
set(libs comctl32 gdi32 opengl32)

add_library(pj64-parallel-rdp SHARED ${src_files} ${res_files})
target_link_libraries(pj64-parallel-rdp PUBLIC parallel-rdp-standalone ${libs})
target_compile_options(pj64-parallel-rdp PRIVATE ${PJ64_PARALLEL_RDP_CXX_FLAGS})
target_include_directories(pj64-parallel-rdp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/parallel-rdp)
target_compile_definitions(pj64-parallel-rdp PRIVATE NOMINMAX)
set_target_properties(pj64-parallel-rdp PROPERTIES PREFIX "" SUFFIX ".dll")
