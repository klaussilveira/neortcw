# iortcw: multiplayer
option(BUILD_CLIENT "Build MP client executable" ON)
option(BUILD_SERVER "Build MP dedicated server executable" ON)
option(BUILD_GAME_SO "Build MP game shared libraries" ON)
option(BUILD_GAME_QVM "Build MP game QVM bytecode" OFF)
option(BUILD_RENDERER_REND2 "Build rend2 (OpenGL 2) renderer" ON)

option(USE_CURL "Use libcurl for HTTP downloads" ON)
option(USE_CURL_DLOPEN "dlopen libcurl at runtime" ON)

# --- CURL (MP-specific, SP defaults to off) ---
if(USE_CURL AND NOT USE_CURL_DLOPEN)
    find_package(CURL)
endif()

# MP version string
set(MP_VERSION "1.51d-MP")
if(GIT_REV)
    set(MP_VERSION "${MP_VERSION}_GIT_${GIT_REV}")
endif()
message(STATUS "MP Version: ${MP_VERSION}")

# MP compile flags
set(MP_BASE_CFLAGS
    -Wall
    -fno-strict-aliasing
    -pipe
    -DUSE_ICON
    "-DARCH_STRING=\"${FILE_ARCH}\""
    "-DPRODUCT_VERSION=\"${MP_VERSION}\""
    -Wformat=2
    -Wformat-security
    -Wno-format-nonliteral
    -Wstrict-aliasing=2
    -Wmissing-format-attribute
    -Wdisabled-optimization
)

if(ARCH STREQUAL "x86")
    list(APPEND MP_BASE_CFLAGS -m32)
elseif(ARCH STREQUAL "x86_64")
    list(APPEND MP_BASE_CFLAGS -m64)
endif()

if(NOT HAVE_VM_COMPILED)
    list(APPEND MP_BASE_CFLAGS -DNO_VM_COMPILED)
endif()

if(PLATFORM STREQUAL "darwin")
    list(APPEND MP_BASE_CFLAGS -fno-common -D_THREAD_SAFE=1)
endif()

# Source file lists

set(CLIENT_SRC
    code/client/cl_cgame.c
    code/client/cl_cin.c
    code/client/cl_console.c
    code/client/cl_input.c
    code/client/cl_keys.c
    code/client/cl_main.c
    code/client/cl_net_chan.c
    code/client/cl_parse.c
    code/client/cl_scrn.c
    code/client/cl_ui.c
    code/client/cl_avi.c
    code/qcommon/cm_load.c
    code/qcommon/cm_patch.c
    code/qcommon/cm_polylib.c
    code/qcommon/cm_test.c
    code/qcommon/cm_trace.c
    code/qcommon/cmd.c
    code/qcommon/common.c
    code/qcommon/cvar.c
    code/qcommon/files.c
    code/qcommon/md4.c
    code/qcommon/md5.c
    code/qcommon/pbmd5.c
    code/qcommon/msg.c
    code/qcommon/net_chan.c
    code/qcommon/net_ip.c
    code/qcommon/huffman.c
    code/qcommon/q_math.c
    code/qcommon/q_shared.c
    ${PROJECT_SOURCE_DIR}/libs/zlib-1.3.2/contrib/minizip/unzip.c
    ${PROJECT_SOURCE_DIR}/libs/zlib-1.3.2/contrib/minizip/ioapi.c
    code/qcommon/puff.c
    code/qcommon/vm.c
    code/qcommon/vm_interpreted.c
    code/client/snd_altivec.c
    code/client/snd_adpcm.c
    code/client/snd_dma.c
    code/client/snd_mem.c
    code/client/snd_mix.c
    code/client/snd_wavelet.c
    code/client/snd_main.c
    code/client/snd_codec.c
    code/client/snd_codec_wav.c
    code/client/snd_codec_ogg.c
    code/client/snd_codec_opus.c
    code/client/qal.c
    code/client/snd_openal.c
    code/client/cl_curl.c
    code/server/sv_bot.c
    code/server/sv_ccmds.c
    code/server/sv_client.c
    code/server/sv_game.c
    code/server/sv_init.c
    code/server/sv_main.c
    code/server/sv_net_chan.c
    code/server/sv_snapshot.c
    code/server/sv_world.c
    code/botlib/be_aas_bspq3.c
    code/botlib/be_aas_cluster.c
    code/botlib/be_aas_debug.c
    code/botlib/be_aas_entity.c
    code/botlib/be_aas_file.c
    code/botlib/be_aas_main.c
    code/botlib/be_aas_move.c
    code/botlib/be_aas_optimize.c
    code/botlib/be_aas_reach.c
    code/botlib/be_aas_route.c
    code/botlib/be_aas_routealt.c
    code/botlib/be_aas_routetable.c
    code/botlib/be_aas_sample.c
    code/botlib/be_ai_char.c
    code/botlib/be_ai_chat.c
    code/botlib/be_ai_gen.c
    code/botlib/be_ai_goal.c
    code/botlib/be_ai_move.c
    code/botlib/be_ai_weap.c
    code/botlib/be_ai_weight.c
    code/botlib/be_ea.c
    code/botlib/be_interface.c
    code/botlib/l_crc.c
    code/botlib/l_libvar.c
    code/botlib/l_log.c
    code/botlib/l_memory.c
    code/botlib/l_precomp.c
    code/botlib/l_script.c
    code/botlib/l_struct.c
    code/sdl/sdl_input.c
    code/sdl/sdl_snd.c
    code/sys/con_log.c
    code/sys/sys_main.c
)

set(SPLINES_SRC
    code/splines/math_angles.cpp
    code/splines/math_matrix.cpp
    code/splines/math_quaternion.cpp
    code/splines/math_vector.cpp
    code/splines/q_parse.cpp
    code/splines/splines.cpp
    code/splines/util_str.cpp
)

# Platform-specific sources
if(WIN32)
    list(APPEND CLIENT_SRC code/sys/sys_win32.c code/sys/con_passive.c)
else()
    list(APPEND CLIENT_SRC code/sys/sys_unix.c code/sys/con_tty.c)
endif()
if(APPLE)
    list(APPEND CLIENT_SRC code/sys/sys_osx.m)
endif()

# Architecture-specific assembly
if(ARCH STREQUAL "x86")
    list(
        APPEND CLIENT_SRC
        code/asm/snd_mixa.s
        code/asm/matha.s
        code/asm/snapvector.c
        code/asm/ftola.c
    )
elseif(ARCH STREQUAL "x86_64")
    list(APPEND CLIENT_SRC code/asm/snapvector.c code/asm/ftola.c)
endif()

# VM compiled support
if(HAVE_VM_COMPILED)
    if(ARCH MATCHES "^(x86|x86_64)$")
        list(APPEND CLIENT_SRC code/qcommon/vm_x86.c)
    elseif(ARCH STREQUAL "ppc")
        list(
            APPEND CLIENT_SRC
            code/qcommon/vm_powerpc.c
            code/qcommon/vm_powerpc_asm.c
        )
    elseif(ARCH STREQUAL "sparc")
        list(APPEND CLIENT_SRC code/qcommon/vm_sparc.c)
    elseif(ARCH STREQUAL "armv7l")
        list(APPEND CLIENT_SRC code/qcommon/vm_armv7l.c)
    endif()
endif()

if(USE_MUMBLE)
    list(APPEND CLIENT_SRC code/client/libmumblelink.c)
endif()

# --- Renderer OpenGL1 ---
set(RENDERER_SRC
    code/renderer/tr_altivec.c
    code/renderer/tr_animation.c
    code/renderer/tr_backend.c
    code/renderer/tr_bsp.c
    code/renderer/tr_cmds.c
    code/renderer/tr_cmesh.c
    code/renderer/tr_curve.c
    code/renderer/tr_flares.c
    code/renderer/tr_font.c
    code/renderer/tr_image.c
    code/renderer/tr_image_bmp.c
    code/renderer/tr_image_jpg.c
    code/renderer/tr_image_pcx.c
    code/renderer/tr_image_png.c
    code/renderer/tr_image_tga.c
    code/renderer/tr_init.c
    code/renderer/tr_light.c
    code/renderer/tr_main.c
    code/renderer/tr_marks.c
    code/renderer/tr_mesh.c
    code/renderer/tr_model.c
    code/renderer/tr_model_iqm.c
    code/renderer/tr_noise.c
    code/renderer/tr_scene.c
    code/renderer/tr_shade.c
    code/renderer/tr_shade_calc.c
    code/renderer/tr_shader.c
    code/renderer/tr_shadows.c
    code/renderer/tr_sky.c
    code/renderer/tr_surface.c
    code/renderer/tr_world.c
    code/sdl/sdl_gamma.c
    code/sdl/sdl_glimp.c
)

if(USE_BLOOM)
    list(APPEND RENDERER_SRC code/renderer/tr_bloom.c)
endif()

set(RENDERER_COMMON_SRC
    code/qcommon/q_shared.c
    code/qcommon/puff.c
    code/qcommon/q_math.c
    code/renderer/tr_subs.c
)

# --- Renderer rend2 ---
set(REND2_SRC
    code/rend2/tr_animation.c
    code/rend2/tr_backend.c
    code/rend2/tr_bsp.c
    code/rend2/tr_cmds.c
    code/rend2/tr_curve.c
    code/rend2/tr_dsa.c
    code/rend2/tr_extramath.c
    code/rend2/tr_extensions.c
    code/rend2/tr_fbo.c
    code/rend2/tr_flares.c
    code/rend2/tr_font.c
    code/rend2/tr_glsl.c
    code/rend2/tr_image.c
    code/rend2/tr_image_bmp.c
    code/rend2/tr_image_jpg.c
    code/rend2/tr_image_pcx.c
    code/rend2/tr_image_png.c
    code/rend2/tr_image_tga.c
    code/rend2/tr_image_dds.c
    code/rend2/tr_init.c
    code/rend2/tr_light.c
    code/rend2/tr_main.c
    code/rend2/tr_marks.c
    code/rend2/tr_mesh.c
    code/rend2/tr_model.c
    code/rend2/tr_model_iqm.c
    code/rend2/tr_noise.c
    code/rend2/tr_postprocess.c
    code/rend2/tr_scene.c
    code/rend2/tr_shade.c
    code/rend2/tr_shade_calc.c
    code/rend2/tr_shader.c
    code/rend2/tr_shadows.c
    code/rend2/tr_sky.c
    code/rend2/tr_surface.c
    code/rend2/tr_vbo.c
    code/rend2/tr_world.c
    code/sdl/sdl_gamma.c
    code/sdl/sdl_glimp.c
)

set(GLSL_SHADERS
    bokeh_fp
    bokeh_vp
    calclevels4x_fp
    calclevels4x_vp
    depthblur_fp
    depthblur_vp
    dlight_fp
    dlight_vp
    down4x_fp
    down4x_vp
    fogpass_fp
    fogpass_vp
    generic_fp
    generic_vp
    greyscale_fp
    greyscale_vp
    lightall_fp
    lightall_vp
    pshadow_fp
    pshadow_vp
    shadowfill_fp
    shadowfill_vp
    shadowmask_fp
    shadowmask_vp
    ssao_fp
    ssao_vp
    texturecolor_fp
    texturecolor_vp
    tonemap_fp
    tonemap_vp
)

# --- Dedicated server sources ---
set(DEDICATED_SRC
    code/server/sv_bot.c
    code/server/sv_client.c
    code/server/sv_ccmds.c
    code/server/sv_game.c
    code/server/sv_init.c
    code/server/sv_main.c
    code/server/sv_net_chan.c
    code/server/sv_snapshot.c
    code/server/sv_world.c
    code/qcommon/cm_load.c
    code/qcommon/cm_patch.c
    code/qcommon/cm_polylib.c
    code/qcommon/cm_test.c
    code/qcommon/cm_trace.c
    code/qcommon/cmd.c
    code/qcommon/common.c
    code/qcommon/cvar.c
    code/qcommon/files.c
    code/qcommon/md4.c
    code/qcommon/msg.c
    code/qcommon/net_chan.c
    code/qcommon/net_ip.c
    code/qcommon/huffman.c
    code/qcommon/q_math.c
    code/qcommon/q_shared.c
    ${PROJECT_SOURCE_DIR}/libs/zlib-1.3.2/contrib/minizip/unzip.c
    ${PROJECT_SOURCE_DIR}/libs/zlib-1.3.2/contrib/minizip/ioapi.c
    code/qcommon/vm.c
    code/qcommon/vm_interpreted.c
    code/botlib/be_aas_bspq3.c
    code/botlib/be_aas_cluster.c
    code/botlib/be_aas_debug.c
    code/botlib/be_aas_entity.c
    code/botlib/be_aas_file.c
    code/botlib/be_aas_main.c
    code/botlib/be_aas_move.c
    code/botlib/be_aas_optimize.c
    code/botlib/be_aas_reach.c
    code/botlib/be_aas_route.c
    code/botlib/be_aas_routealt.c
    code/botlib/be_aas_routetable.c
    code/botlib/be_aas_sample.c
    code/botlib/be_ai_char.c
    code/botlib/be_ai_chat.c
    code/botlib/be_ai_gen.c
    code/botlib/be_ai_goal.c
    code/botlib/be_ai_move.c
    code/botlib/be_ai_weap.c
    code/botlib/be_ai_weight.c
    code/botlib/be_ea.c
    code/botlib/be_interface.c
    code/botlib/l_crc.c
    code/botlib/l_libvar.c
    code/botlib/l_log.c
    code/botlib/l_memory.c
    code/botlib/l_precomp.c
    code/botlib/l_script.c
    code/botlib/l_struct.c
    code/null/null_client.c
    code/null/null_input.c
    code/null/null_snddma.c
    code/sys/con_log.c
    code/sys/sys_main.c
)

if(WIN32)
    list(APPEND DEDICATED_SRC code/sys/sys_win32.c code/sys/con_win32.c)
else()
    list(APPEND DEDICATED_SRC code/sys/sys_unix.c code/sys/con_tty.c)
endif()
if(APPLE)
    list(APPEND DEDICATED_SRC code/sys/sys_osx.m)
endif()

if(ARCH STREQUAL "x86")
    list(
        APPEND DEDICATED_SRC
        code/asm/matha.s
        code/asm/snapvector.c
        code/asm/ftola.c
    )
elseif(ARCH STREQUAL "x86_64")
    list(APPEND DEDICATED_SRC code/asm/snapvector.c code/asm/ftola.c)
endif()

if(HAVE_VM_COMPILED)
    if(ARCH MATCHES "^(x86|x86_64)$")
        list(APPEND DEDICATED_SRC code/qcommon/vm_x86.c)
    elseif(ARCH STREQUAL "ppc")
        list(
            APPEND DEDICATED_SRC
            code/qcommon/vm_powerpc.c
            code/qcommon/vm_powerpc_asm.c
        )
    elseif(ARCH STREQUAL "sparc")
        list(APPEND DEDICATED_SRC code/qcommon/vm_sparc.c)
    elseif(ARCH STREQUAL "armv7l")
        list(APPEND DEDICATED_SRC code/qcommon/vm_armv7l.c)
    endif()
endif()

# --- Game module sources ---
set(CGAME_SRC
    code/cgame/cg_main.c
    code/game/bg_animation.c
    code/game/bg_misc.c
    code/game/bg_pmove.c
    code/game/bg_slidemove.c
    code/game/bg_lib.c
    code/cgame/cg_consolecmds.c
    code/cgame/cg_draw.c
    code/cgame/cg_drawtools.c
    code/cgame/cg_effects.c
    code/cgame/cg_ents.c
    code/cgame/cg_event.c
    code/cgame/cg_flamethrower.c
    code/cgame/cg_info.c
    code/cgame/cg_localents.c
    code/cgame/cg_marks.c
    code/cgame/cg_newdraw.c
    code/cgame/cg_particles.c
    code/cgame/cg_players.c
    code/cgame/cg_playerstate.c
    code/cgame/cg_predict.c
    code/cgame/cg_scoreboard.c
    code/cgame/cg_servercmds.c
    code/cgame/cg_snapshot.c
    code/cgame/cg_sound.c
    code/cgame/cg_spawn.c
    code/cgame/cg_trails.c
    code/cgame/cg_view.c
    code/cgame/cg_weapons.c
    code/ui/ui_shared.c
    code/cgame/cg_syscalls.c
    code/qcommon/q_math.c
    code/qcommon/q_shared.c
)

set(GAME_SRC
    code/game/g_main.c
    code/game/ai_cast.c
    code/game/ai_cast_characters.c
    code/game/ai_cast_debug.c
    code/game/ai_cast_events.c
    code/game/ai_cast_fight.c
    code/game/ai_cast_func_attack.c
    code/game/ai_cast_func_boss1.c
    code/game/ai_cast_funcs.c
    code/game/ai_cast_script_actions.c
    code/game/ai_cast_script.c
    code/game/ai_cast_script_ents.c
    code/game/ai_cast_sight.c
    code/game/ai_cast_think.c
    code/game/ai_chat.c
    code/game/ai_cmd.c
    code/game/ai_dmnet.c
    code/game/ai_dmq3.c
    code/game/ai_main.c
    code/game/ai_team.c
    code/game/bg_animation.c
    code/game/bg_misc.c
    code/game/bg_pmove.c
    code/game/bg_slidemove.c
    code/game/bg_lib.c
    code/game/g_active.c
    code/game/g_alarm.c
    code/game/g_antilag.c
    code/game/g_bot.c
    code/game/g_client.c
    code/game/g_cmds.c
    code/game/g_combat.c
    code/game/g_items.c
    code/game/g_mem.c
    code/game/g_misc.c
    code/game/g_missile.c
    code/game/g_mover.c
    code/game/g_props.c
    code/game/g_script_actions.c
    code/game/g_script.c
    code/game/g_session.c
    code/game/g_spawn.c
    code/game/g_svcmds.c
    code/game/g_target.c
    code/game/g_team.c
    code/game/g_tramcar.c
    code/game/g_trigger.c
    code/game/g_utils.c
    code/game/g_weapon.c
    code/game/g_syscalls.c
    code/qcommon/q_math.c
    code/qcommon/q_shared.c
)

set(UI_SRC
    code/ui/ui_main.c
    code/ui/ui_atoms.c
    code/ui/ui_gameinfo.c
    code/ui/ui_players.c
    code/ui/ui_shared.c
    code/game/bg_misc.c
    code/game/bg_lib.c
    code/ui/ui_syscalls.c
    code/qcommon/q_math.c
    code/qcommon/q_shared.c
)

# Botlib files require -DBOTLIB
file(GLOB BOTLIB_SOURCES code/botlib/*.c)
set_source_files_properties(
    ${BOTLIB_SOURCES}
    PROPERTIES COMPILE_DEFINITIONS "BOTLIB"
)

# Build targets

# --- Stringify tool (for rend2 GLSL) ---
if(BUILD_CLIENT AND BUILD_RENDERER_REND2)
    if(NOT TARGET stringify)
        add_executable(stringify code/tools/stringify.c)
        set_target_properties(
            stringify
            PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools"
        )
    endif()

    set(MP_GLSL_GENERATED_SRC "")
    foreach(SHADER ${GLSL_SHADERS})
        set(GLSL_IN
            "${CMAKE_CURRENT_SOURCE_DIR}/code/rend2/glsl/${SHADER}.glsl"
        )
        set(GLSL_OUT "${CMAKE_BINARY_DIR}/mp_rend2/glsl/${SHADER}.c")
        add_custom_command(
            OUTPUT ${GLSL_OUT}
            COMMAND
                ${CMAKE_COMMAND} -E make_directory
                "${CMAKE_BINARY_DIR}/mp_rend2/glsl"
            COMMAND stringify ${GLSL_IN} ${GLSL_OUT}
            DEPENDS stringify ${GLSL_IN}
            COMMENT "Stringify MP ${SHADER}.glsl"
        )
        list(APPEND MP_GLSL_GENERATED_SRC ${GLSL_OUT})
    endforeach()
endif()

# --- Renderer OpenGL 1 (shared library) ---
if(BUILD_CLIENT AND USE_RENDERER_DLOPEN)
    add_library(
        renderer_mp_opengl1
        SHARED
        ${RENDERER_SRC}
        ${RENDERER_COMMON_SRC}
    )
    set_target_properties(
        renderer_mp_opengl1
        PROPERTIES
            OUTPUT_NAME "renderer_mp_opengl1_${SHLIBNAME}"
            PREFIX ""
            SUFFIX ""
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
    target_include_directories(
        renderer_mp_opengl1
        PRIVATE
            code/renderer
            code/sdl
            code/qcommon
            code/client
            code/server
            code/sys
    )
    target_compile_options(
        renderer_mp_opengl1
        PRIVATE ${MP_BASE_CFLAGS} ${SHLIBCFLAGS}
    )
    target_compile_definitions(renderer_mp_opengl1 PRIVATE USE_RENDERER_DLOPEN)
    if(USE_BLOOM)
        target_compile_definitions(renderer_mp_opengl1 PRIVATE USE_BLOOM)
    endif()
    if(USE_INTERNAL_JPEG)
        target_compile_definitions(
            renderer_mp_opengl1
            PRIVATE USE_INTERNAL_JPEG
        )
    endif()
    if(USE_FREETYPE)
        target_compile_definitions(renderer_mp_opengl1 PRIVATE BUILD_FREETYPE)
    endif()
    link_renderer_libs(renderer_mp_opengl1)
    link_wolf_libs(renderer_mp_opengl1)

    if(NOT WIN32)
        target_link_libraries(renderer_mp_opengl1 PRIVATE m)
    endif()
    if(PLATFORM STREQUAL "linux")
        target_link_libraries(renderer_mp_opengl1 PRIVATE dl pthread)
    endif()
endif()

# --- Renderer rend2 (shared library) ---
if(BUILD_CLIENT AND BUILD_RENDERER_REND2 AND USE_RENDERER_DLOPEN)
    add_library(
        renderer_mp_rend2
        SHARED
        ${REND2_SRC}
        ${RENDERER_COMMON_SRC}
        ${MP_GLSL_GENERATED_SRC}
    )
    set_target_properties(
        renderer_mp_rend2
        PROPERTIES
            OUTPUT_NAME "renderer_mp_rend2_${SHLIBNAME}"
            PREFIX ""
            SUFFIX ""
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
    target_include_directories(
        renderer_mp_rend2
        PRIVATE
            code/rend2
            code/renderer
            code/sdl
            code/qcommon
            code/client
            code/server
            code/sys
            "${CMAKE_BINARY_DIR}/mp_rend2/glsl"
    )
    target_compile_options(
        renderer_mp_rend2
        PRIVATE ${MP_BASE_CFLAGS} ${SHLIBCFLAGS}
    )
    target_compile_definitions(renderer_mp_rend2 PRIVATE USE_RENDERER_DLOPEN)
    if(USE_INTERNAL_JPEG)
        target_compile_definitions(renderer_mp_rend2 PRIVATE USE_INTERNAL_JPEG)
    endif()
    if(USE_FREETYPE)
        target_compile_definitions(renderer_mp_rend2 PRIVATE BUILD_FREETYPE)
    endif()
    link_renderer_libs(renderer_mp_rend2)
    link_wolf_libs(renderer_mp_rend2)

    if(NOT WIN32)
        target_link_libraries(renderer_mp_rend2 PRIVATE m)
    endif()
    if(PLATFORM STREQUAL "linux")
        target_link_libraries(renderer_mp_rend2 PRIVATE dl pthread)
    endif()
endif()

# --- Client executable ---
if(BUILD_CLIENT)
    if(USE_RENDERER_DLOPEN)
        set(CLIENT_ALL_SRC ${CLIENT_SRC} ${SPLINES_SRC})
    else()
        set(CLIENT_ALL_SRC ${CLIENT_SRC} ${SPLINES_SRC} ${RENDERER_SRC})
    endif()

    add_executable(iowolfmp ${CLIENT_ALL_SRC})
    set_target_properties(
        iowolfmp
        PROPERTIES
            OUTPUT_NAME "iowolfmp.${ARCH}"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
    target_include_directories(
        iowolfmp
        PRIVATE
            code/client
            code/server
            code/renderer
            code/rend2
            code/qcommon
            code/sdl
            code/sys
            code/botlib
            code/splines
            ${PROJECT_SOURCE_DIR}/libs/zlib-1.3.2
            ${PROJECT_SOURCE_DIR}/libs/zlib-1.3.2/contrib/minizip
            ${PROJECT_SOURCE_DIR}/libs
            ${SDL2_INCLUDE_DIR}
    )
    target_compile_options(iowolfmp PRIVATE ${MP_BASE_CFLAGS})
    add_wolf_definitions(iowolfmp)
    if(USE_CURL)
        target_compile_definitions(iowolfmp PRIVATE USE_CURL)
        if(USE_CURL_DLOPEN)
            target_compile_definitions(iowolfmp PRIVATE USE_CURL_DLOPEN)
        endif()
    endif()
    link_wolf_libs(iowolfmp)
    link_client_codec_libs(iowolfmp)

    target_link_libraries(iowolfmp PRIVATE ${SDL2_LIBRARY})

    if(NOT USE_RENDERER_DLOPEN)
        link_renderer_libs(iowolfmp)
    endif()

    if(USE_OPENAL AND NOT USE_OPENAL_DLOPEN AND OPENAL_FOUND)
        target_link_libraries(iowolfmp PRIVATE ${OPENAL_LIBRARY})
        target_include_directories(iowolfmp PRIVATE ${OPENAL_INCLUDE_DIR})
    endif()

    if(USE_CURL AND NOT USE_CURL_DLOPEN AND CURL_FOUND)
        target_link_libraries(iowolfmp PRIVATE ${CURL_LIBRARIES})
        target_include_directories(iowolfmp PRIVATE ${CURL_INCLUDE_DIRS})
    endif()

    if(PLATFORM STREQUAL "linux")
        target_link_libraries(iowolfmp PRIVATE dl m pthread)
        if(USE_MUMBLE)
            target_link_libraries(iowolfmp PRIVATE rt)
        endif()
    elseif(PLATFORM STREQUAL "freebsd")
        target_link_libraries(iowolfmp PRIVATE m pthread)
    elseif(PLATFORM STREQUAL "darwin")
        target_link_libraries(
            iowolfmp
            PRIVATE "-framework IOKit" "-framework Cocoa"
        )
    elseif(WIN32)
        target_link_libraries(
            iowolfmp
            PRIVATE ws2_32 winmm psapi gdi32 ole32
        )
    endif()
endif()

# --- Dedicated server ---
if(BUILD_SERVER)
    add_executable(iowolfded ${DEDICATED_SRC})
    set_target_properties(
        iowolfded
        PROPERTIES
            OUTPUT_NAME "iowolfded.${ARCH}"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
    target_include_directories(
        iowolfded
        PRIVATE
            code/server
            code/qcommon
            code/sys
            code/botlib
            code/null
            ${PROJECT_SOURCE_DIR}/libs/zlib-1.3.2
            ${PROJECT_SOURCE_DIR}/libs/zlib-1.3.2/contrib/minizip
    )
    target_compile_options(iowolfded PRIVATE ${MP_BASE_CFLAGS})
    target_compile_definitions(iowolfded PRIVATE DEDICATED)

    if(USE_VOIP)
        target_compile_definitions(iowolfded PRIVATE USE_VOIP)
    endif()

    link_wolf_libs(iowolfded)

    if(PLATFORM STREQUAL "linux")
        target_link_libraries(iowolfded PRIVATE dl m pthread)
    elseif(PLATFORM STREQUAL "freebsd")
        target_link_libraries(iowolfded PRIVATE m pthread)
    elseif(PLATFORM STREQUAL "darwin")
        target_link_libraries(iowolfded PRIVATE "-framework Cocoa")
    elseif(WIN32)
        target_link_libraries(iowolfded PRIVATE ws2_32 winmm psapi)
    endif()
endif()

# --- Game modules (shared libraries) ---
if(BUILD_GAME_SO)
    # cgame
    add_library(cgame_mp SHARED ${CGAME_SRC})
    set_target_properties(
        cgame_mp
        PROPERTIES
            OUTPUT_NAME "cgame${MP_GAMEMOD_SHLIBNAME}"
            PREFIX ""
            SUFFIX ""
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/main"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/main"
    )
    target_include_directories(
        cgame_mp
        PRIVATE code/cgame code/game code/ui code/qcommon
    )
    target_compile_options(cgame_mp PRIVATE ${MP_BASE_CFLAGS} ${SHLIBCFLAGS})
    target_compile_definitions(cgame_mp PRIVATE CGAMEDLL CGAME)
    if(NOT WIN32)
        target_link_libraries(cgame_mp PRIVATE m)
    endif()

    # qagame
    add_library(qagame_mp SHARED ${GAME_SRC})
    set_target_properties(
        qagame_mp
        PROPERTIES
            OUTPUT_NAME "qagame${MP_GAMEMOD_SHLIBNAME}"
            PREFIX ""
            SUFFIX ""
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/main"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/main"
    )
    target_include_directories(
        qagame_mp
        PRIVATE code/game code/qcommon code/botlib
    )
    target_compile_options(qagame_mp PRIVATE ${MP_BASE_CFLAGS} ${SHLIBCFLAGS})
    target_compile_definitions(qagame_mp PRIVATE GAMEDLL QAGAME)
    if(NOT WIN32)
        target_link_libraries(qagame_mp PRIVATE m)
    endif()

    # ui
    add_library(ui_mp SHARED ${UI_SRC})
    set_target_properties(
        ui_mp
        PROPERTIES
            OUTPUT_NAME "ui${MP_GAMEMOD_SHLIBNAME}"
            PREFIX ""
            SUFFIX ""
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/main"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/main"
    )
    target_include_directories(ui_mp PRIVATE code/ui code/game code/qcommon)
    target_compile_options(ui_mp PRIVATE ${MP_BASE_CFLAGS} ${SHLIBCFLAGS})
    target_compile_definitions(ui_mp PRIVATE UI)
    if(NOT WIN32)
        target_link_libraries(ui_mp PRIVATE m)
    endif()
endif()

# QVM Build (optional, off by default)
if(BUILD_GAME_QVM)
    add_executable(
        lburg
        code/tools/lcc/lburg/lburg.c
        code/tools/lcc/lburg/gram.c
    )
    target_include_directories(
        lburg
        PRIVATE code/tools/lcc/lburg code/tools/lcc/src
    )
    target_compile_options(lburg PRIVATE -g -Wall -fno-strict-aliasing)
    set_target_properties(
        lburg
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools"
    )

    add_custom_command(
        OUTPUT "${CMAKE_BINARY_DIR}/tools/dagcheck.c"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/tools"
        COMMAND
            lburg "${CMAKE_CURRENT_SOURCE_DIR}/code/tools/lcc/src/dagcheck.md"
            "${CMAKE_BINARY_DIR}/tools/dagcheck.c"
        DEPENDS
            lburg
            "${CMAKE_CURRENT_SOURCE_DIR}/code/tools/lcc/src/dagcheck.md"
        COMMENT "LBURG dagcheck.md"
    )

    add_executable(
        q3rcc
        code/tools/lcc/src/alloc.c
        code/tools/lcc/src/bind.c
        code/tools/lcc/src/bytecode.c
        code/tools/lcc/src/dag.c
        "${CMAKE_BINARY_DIR}/tools/dagcheck.c"
        code/tools/lcc/src/decl.c
        code/tools/lcc/src/enode.c
        code/tools/lcc/src/error.c
        code/tools/lcc/src/event.c
        code/tools/lcc/src/expr.c
        code/tools/lcc/src/gen.c
        code/tools/lcc/src/init.c
        code/tools/lcc/src/inits.c
        code/tools/lcc/src/input.c
        code/tools/lcc/src/lex.c
        code/tools/lcc/src/list.c
        code/tools/lcc/src/main.c
        code/tools/lcc/src/null.c
        code/tools/lcc/src/output.c
        code/tools/lcc/src/prof.c
        code/tools/lcc/src/profio.c
        code/tools/lcc/src/simp.c
        code/tools/lcc/src/stmt.c
        code/tools/lcc/src/string.c
        code/tools/lcc/src/sym.c
        code/tools/lcc/src/symbolic.c
        code/tools/lcc/src/trace.c
        code/tools/lcc/src/tree.c
        code/tools/lcc/src/types.c
    )
    target_include_directories(
        q3rcc
        PRIVATE code/tools/lcc/src code/tools/lcc/lburg
    )
    target_compile_options(q3rcc PRIVATE -g -Wall -fno-strict-aliasing)
    set_target_properties(
        q3rcc
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools"
    )

    add_executable(
        q3cpp
        code/tools/lcc/cpp/cpp.c
        code/tools/lcc/cpp/lex.c
        code/tools/lcc/cpp/nlist.c
        code/tools/lcc/cpp/tokens.c
        code/tools/lcc/cpp/macro.c
        code/tools/lcc/cpp/eval.c
        code/tools/lcc/cpp/include.c
        code/tools/lcc/cpp/hideset.c
        code/tools/lcc/cpp/getopt.c
        code/tools/lcc/cpp/unix.c
    )
    target_include_directories(q3cpp PRIVATE code/tools/lcc/cpp)
    target_compile_options(q3cpp PRIVATE -g -Wall -fno-strict-aliasing)
    set_target_properties(
        q3cpp
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools"
    )

    add_executable(q3lcc code/tools/lcc/etc/lcc.c code/tools/lcc/etc/bytecode.c)
    target_include_directories(
        q3lcc
        PRIVATE code/tools/lcc/etc code/tools/lcc/src
    )
    target_compile_options(
        q3lcc
        PRIVATE
            -g
            -Wall
            -fno-strict-aliasing
            "-DTEMPDIR=\"/tmp\""
            "-DSYSTEM=\"\""
            "-DARCH_STRING=\"${ARCH}\""
    )
    set_target_properties(
        q3lcc
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools"
    )

    add_executable(q3asm code/tools/asm/q3asm.c code/tools/asm/cmdlib.c)
    target_include_directories(q3asm PRIVATE code/tools/asm)
    target_compile_options(q3asm PRIVATE -g -Wall -fno-strict-aliasing)
    set_target_properties(
        q3asm
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools"
    )
endif()

# Install rules
if(BUILD_CLIENT)
    install(TARGETS iowolfmp RUNTIME DESTINATION bin)
    if(USE_RENDERER_DLOPEN)
        install(TARGETS renderer_mp_opengl1 LIBRARY DESTINATION bin)
        if(BUILD_RENDERER_REND2)
            install(TARGETS renderer_mp_rend2 LIBRARY DESTINATION bin)
        endif()
    endif()
endif()

if(BUILD_SERVER)
    install(TARGETS iowolfded RUNTIME DESTINATION bin)
endif()

if(BUILD_GAME_SO)
    install(TARGETS cgame_mp qagame_mp ui_mp LIBRARY DESTINATION bin/main)
endif()
