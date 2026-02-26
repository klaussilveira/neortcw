# iortcw: Enemy Territory
option(ET_BUILD_CLIENT "Build ET client executable" ON)
option(ET_BUILD_SERVER "Build ET dedicated server executable" OFF)
option(ET_BUILD_GAME_SO "Build ET game shared libraries" ON)

# ET version string
set(ET_VERSION "1.51d-ET")
if(GIT_REV)
    set(ET_VERSION "${ET_VERSION}_GIT_${GIT_REV}")
endif()
message(STATUS "ET Version: ${ET_VERSION}")

# ET compile flags
set(ET_BASE_CFLAGS
    -Wall
    -fno-strict-aliasing
    -pipe
    -DUSE_ICON
    "-DARCH_STRING=\"${FILE_ARCH}\""
    "-DPRODUCT_VERSION=\"${ET_VERSION}\""
    -Wformat=2
    -Wformat-security
    -Wno-format-nonliteral
    -Wstrict-aliasing=2
    -Wmissing-format-attribute
    -Wdisabled-optimization
)

if(EMSCRIPTEN)
    list(FILTER ET_BASE_CFLAGS EXCLUDE REGEX "-m32|-m64|-pipe")
    list(APPEND ET_BASE_CFLAGS -sUSE_SDL=2)
elseif(ARCH STREQUAL "x86")
    list(APPEND ET_BASE_CFLAGS -m32)
elseif(ARCH STREQUAL "x86_64")
    list(APPEND ET_BASE_CFLAGS -m64)
endif()

# ET's vm_x86.c is 32-bit only; on other arches, use DLL_ONLY stubs
if(NOT ARCH STREQUAL "x86")
    list(APPEND ET_BASE_CFLAGS -DDLL_ONLY)
endif()

if(PLATFORM STREQUAL "darwin")
    list(APPEND ET_BASE_CFLAGS -fno-common -D_THREAD_SAFE=1)
endif()

# Find system curl
find_package(CURL)
if(NOT CURL_FOUND)
    # Try pkg-config
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(CURL libcurl)
    endif()
endif()

# Source file lists

# Client/Engine sources
set(ET_CLIENT_SRC
    src/client/cl_cgame.c
    src/client/cl_cin.c
    src/client/cl_console.c
    src/client/cl_input.c
    src/client/cl_keys.c
    src/client/cl_main.c
    src/client/cl_net_chan.c
    src/client/cl_parse.c
    src/client/cl_scrn.c
    src/client/cl_ui.c
    src/qcommon/cm_load.c
    src/qcommon/cm_patch.c
    src/qcommon/cm_polylib.c
    src/qcommon/cm_test.c
    src/qcommon/cm_trace.c
    src/qcommon/cmd.c
    src/qcommon/common.c
    src/qcommon/cvar.c
    src/qcommon/files.c
    src/qcommon/md4.c
    src/qcommon/msg.c
    src/qcommon/net_chan.c
    src/qcommon/huffman.c
    src/game/q_math.c
    src/game/q_shared.c
    src/qcommon/unzip.c
    src/qcommon/vm.c
    src/qcommon/vm_interpreted.c
    src/client/snd_adpcm.c
    src/client/snd_dma.c
    src/client/snd_mem.c
    src/client/snd_mix.c
    src/client/snd_wavelet.c
    src/qcommon/dl_main_curl.c
    src/server/sv_bot.c
    src/server/sv_ccmds.c
    src/server/sv_client.c
    src/server/sv_game.c
    src/server/sv_init.c
    src/server/sv_main.c
    src/server/sv_net_chan.c
    src/server/sv_snapshot.c
    src/server/sv_world.c
    src/botlib/be_aas_bspq3.c
    src/botlib/be_aas_cluster.c
    src/botlib/be_aas_debug.c
    src/botlib/be_aas_entity.c
    src/botlib/be_aas_file.c
    src/botlib/be_aas_main.c
    src/botlib/be_aas_move.c
    src/botlib/be_aas_optimize.c
    src/botlib/be_aas_reach.c
    src/botlib/be_aas_route.c
    src/botlib/be_aas_routealt.c
    src/botlib/be_aas_routetable.c
    src/botlib/be_aas_sample.c
    src/botlib/be_ai_char.c
    src/botlib/be_ai_chat.c
    src/botlib/be_ai_gen.c
    src/botlib/be_ai_goal.c
    src/botlib/be_ai_move.c
    src/botlib/be_ai_weap.c
    src/botlib/be_ai_weight.c
    src/botlib/be_ea.c
    src/botlib/be_interface.c
    src/botlib/l_crc.c
    src/botlib/l_libvar.c
    src/botlib/l_log.c
    src/botlib/l_memory.c
    src/botlib/l_precomp.c
    src/botlib/l_script.c
    src/botlib/l_struct.c
    src/sdl/sdl_input.c
    src/sdl/sdl_snd.c
    src/sys/con_log.c
    src/sys/sys_main.c
)

set(ET_SPLINES_SRC
    src/splines/math_angles.cpp
    src/splines/math_matrix.cpp
    src/splines/math_quaternion.cpp
    src/splines/math_vector.cpp
    src/splines/q_parse.cpp
    src/splines/splines.cpp
    src/splines/util_str.cpp
)

# Platform-specific network code (ET uses its own, not SP/MP's net_ip.c)
if(EMSCRIPTEN)
    list(APPEND ET_CLIENT_SRC src/unix/unix_net.c)
    list(APPEND ET_CLIENT_SRC src/sys/sys_unix.c src/sys/con_passive.c)
elseif(WIN32)
    list(APPEND ET_CLIENT_SRC src/win32/win_net.c)
    list(APPEND ET_CLIENT_SRC src/sys/sys_win32.c src/sys/con_passive.c)
else()
    list(APPEND ET_CLIENT_SRC src/unix/unix_net.c)
    list(APPEND ET_CLIENT_SRC src/sys/sys_unix.c src/sys/con_tty.c)
endif()
if(APPLE)
    list(APPEND ET_CLIENT_SRC src/sys/sys_osx.m)
endif()

# VM compiled support (ET's vm_x86.c is 32-bit only)
if(HAVE_VM_COMPILED)
    if(ARCH STREQUAL "x86")
        list(APPEND ET_CLIENT_SRC src/qcommon/vm_x86.c)
    endif()
endif()

# Renderer OpenGL1
set(ET_RENDERER_SRC
    src/renderer/tr_animation_mdm.c
    src/renderer/tr_animation_mds.c
    src/renderer/tr_backend.c
    src/renderer/tr_bsp.c
    src/renderer/tr_cmds.c
    src/renderer/tr_cmesh.c
    src/renderer/tr_curve.c
    src/renderer/tr_decals.c
    src/renderer/tr_flares.c
    src/renderer/tr_font.c
    src/renderer/tr_image.c
    src/renderer/tr_init.c
    src/renderer/tr_light.c
    src/renderer/tr_main.c
    src/renderer/tr_marks.c
    src/renderer/tr_mesh.c
    src/renderer/tr_model.c
    src/renderer/tr_noise.c
    src/renderer/tr_scene.c
    src/renderer/tr_shade.c
    src/renderer/tr_shade_calc.c
    src/renderer/tr_shader.c
    src/renderer/tr_shadows.c
    src/renderer/tr_sky.c
    src/renderer/tr_surface.c
    src/renderer/tr_world.c
    src/sdl/sdl_gamma.c
    src/sdl/sdl_glimp.c
)

# q_shared.c and q_math.c are already in ET_CLIENT_SRC
set(ET_RENDERER_COMMON_SRC)

# Dedicated server sources
set(ET_DEDICATED_SRC
    src/server/sv_bot.c
    src/server/sv_client.c
    src/server/sv_ccmds.c
    src/server/sv_game.c
    src/server/sv_init.c
    src/server/sv_main.c
    src/server/sv_net_chan.c
    src/server/sv_snapshot.c
    src/server/sv_world.c
    src/qcommon/cm_load.c
    src/qcommon/cm_patch.c
    src/qcommon/cm_polylib.c
    src/qcommon/cm_test.c
    src/qcommon/cm_trace.c
    src/qcommon/cmd.c
    src/qcommon/common.c
    src/qcommon/cvar.c
    src/qcommon/files.c
    src/qcommon/md4.c
    src/qcommon/msg.c
    src/qcommon/net_chan.c
    src/qcommon/huffman.c
    src/game/q_math.c
    src/game/q_shared.c
    src/qcommon/unzip.c
    src/qcommon/vm.c
    src/qcommon/vm_interpreted.c
    src/botlib/be_aas_bspq3.c
    src/botlib/be_aas_cluster.c
    src/botlib/be_aas_debug.c
    src/botlib/be_aas_entity.c
    src/botlib/be_aas_file.c
    src/botlib/be_aas_main.c
    src/botlib/be_aas_move.c
    src/botlib/be_aas_optimize.c
    src/botlib/be_aas_reach.c
    src/botlib/be_aas_route.c
    src/botlib/be_aas_routealt.c
    src/botlib/be_aas_routetable.c
    src/botlib/be_aas_sample.c
    src/botlib/be_ai_char.c
    src/botlib/be_ai_chat.c
    src/botlib/be_ai_gen.c
    src/botlib/be_ai_goal.c
    src/botlib/be_ai_move.c
    src/botlib/be_ai_weap.c
    src/botlib/be_ai_weight.c
    src/botlib/be_ea.c
    src/botlib/be_interface.c
    src/botlib/l_crc.c
    src/botlib/l_libvar.c
    src/botlib/l_log.c
    src/botlib/l_memory.c
    src/botlib/l_precomp.c
    src/botlib/l_script.c
    src/botlib/l_struct.c
    src/null/null_client.c
    src/null/null_input.c
    src/null/null_snddma.c
    src/sys/con_log.c
    src/sys/sys_main.c
)

if(EMSCRIPTEN)
    list(FILTER ET_DEDICATED_SRC EXCLUDE REGEX "unix_net\\.c|sys_unix\\.c|con_tty\\.c")
    list(APPEND ET_DEDICATED_SRC src/unix/unix_net.c)
    list(APPEND ET_DEDICATED_SRC src/sys/sys_unix.c src/sys/con_passive.c)
elseif(WIN32)
    list(APPEND ET_DEDICATED_SRC src/win32/win_net.c)
    list(APPEND ET_DEDICATED_SRC src/sys/sys_win32.c src/sys/con_passive.c)
else()
    list(APPEND ET_DEDICATED_SRC src/unix/unix_net.c)
    list(APPEND ET_DEDICATED_SRC src/sys/sys_unix.c src/sys/con_tty.c)
endif()
if(APPLE)
    list(APPEND ET_DEDICATED_SRC src/sys/sys_osx.m)
endif()

if(HAVE_VM_COMPILED)
    if(ARCH STREQUAL "x86")
        list(APPEND ET_DEDICATED_SRC src/qcommon/vm_x86.c)
    endif()
endif()

# Game module sources

# cgame
set(ET_CGAME_SRC
    src/cgame/cg_main.c
    src/cgame/cg_atmospheric.c
    src/cgame/cg_character.c
    src/cgame/cg_commandmap.c
    src/cgame/cg_consolecmds.c
    src/cgame/cg_debriefing.c
    src/cgame/cg_draw.c
    src/cgame/cg_drawtools.c
    src/cgame/cg_effects.c
    src/cgame/cg_ents.c
    src/cgame/cg_event.c
    src/cgame/cg_fireteamoverlay.c
    src/cgame/cg_fireteams.c
    src/cgame/cg_flamethrower.c
    src/cgame/cg_info.c
    src/cgame/cg_limbopanel.c
    src/cgame/cg_loadpanel.c
    src/cgame/cg_localents.c
    src/cgame/cg_marks.c
    src/cgame/cg_missionbriefing.c
    src/cgame/cg_multiview.c
    src/cgame/cg_newDraw.c
    src/cgame/cg_panelhandling.c
    src/cgame/cg_particles.c
    src/cgame/cg_players.c
    src/cgame/cg_playerstate.c
    src/cgame/cg_polybus.c
    src/cgame/cg_popupmessages.c
    src/cgame/cg_predict.c
    src/cgame/cg_scoreboard.c
    src/cgame/cg_servercmds.c
    src/cgame/cg_snapshot.c
    src/cgame/cg_sound.c
    src/cgame/cg_spawn.c
    src/cgame/cg_statsranksmedals.c
    src/cgame/cg_trails.c
    src/cgame/cg_view.c
    src/cgame/cg_weapons.c
    src/cgame/cg_window.c
    src/cgame/cg_syscalls.c
    src/game/bg_animation.c
    src/game/bg_animgroup.c
    src/game/bg_campaign.c
    src/game/bg_character.c
    src/game/bg_classes.c
    src/game/bg_misc.c
    src/game/bg_pmove.c
    src/game/bg_slidemove.c
    src/game/bg_sscript.c
    src/game/bg_stats.c
    src/game/bg_tracemap.c
    src/ui/ui_shared.c
    src/game/q_math.c
    src/game/q_shared.c
)

# qagame
set(ET_GAME_SRC
    src/game/g_main.c
    src/game/g_active.c
    src/game/g_alarm.c
    src/game/g_antilag.c
    src/game/g_bot.c
    src/game/g_buddy_list.c
    src/game/g_character.c
    src/game/g_client.c
    src/game/g_cmds.c
    src/game/g_cmds_ext.c
    src/game/g_combat.c
    src/game/g_config.c
    src/game/g_fireteams.c
    src/game/g_items.c
    src/game/g_match.c
    src/game/g_mem.c
    src/game/g_misc.c
    src/game/g_missile.c
    src/game/g_mover.c
    src/game/g_multiview.c
    src/game/g_props.c
    src/game/g_referee.c
    src/game/g_save.c
    src/game/g_script.c
    src/game/g_script_actions.c
    src/game/g_session.c
    src/game/g_spawn.c
    src/game/g_stats.c
    src/game/g_sv_entities.c
    src/game/g_svcmds.c
    src/game/g_syscalls.c
    src/game/g_systemmsg.c
    src/game/g_target.c
    src/game/g_team.c
    src/game/g_teammapdata.c
    src/game/g_trigger.c
    src/game/g_utils.c
    src/game/g_vote.c
    src/game/g_weapon.c
    src/game/bg_animation.c
    src/game/bg_animgroup.c
    src/game/bg_campaign.c
    src/game/bg_character.c
    src/game/bg_classes.c
    src/game/bg_misc.c
    src/game/bg_pmove.c
    src/game/bg_slidemove.c
    src/game/bg_sscript.c
    src/game/bg_stats.c
    src/game/bg_tracemap.c
    src/botai/ai_cmd.c
    src/botai/ai_dmgoal_mp.c
    src/botai/ai_dmnet_mp.c
    src/botai/ai_dmq3.c
    src/botai/ai_main.c
    src/botai/ai_script.c
    src/botai/ai_script_actions.c
    src/botai/ai_team.c
    src/game/q_math.c
    src/game/q_shared.c
)

# ui
set(ET_UI_SRC
    src/ui/ui_main.c
    src/ui/ui_atoms.c
    src/ui/ui_gameinfo.c
    src/ui/ui_loadpanel.c
    src/ui/ui_players.c
    src/ui/ui_shared.c
    src/ui/ui_util.c
    src/ui/ui_syscalls.c
    src/game/bg_campaign.c
    src/game/bg_classes.c
    src/game/bg_misc.c
    src/game/q_math.c
    src/game/q_shared.c
)

# Botlib files require -DBOTLIB
file(GLOB ET_BOTLIB_SOURCES src/botlib/*.c)
set_source_files_properties(
    ${ET_BOTLIB_SOURCES}
    PROPERTIES COMPILE_DEFINITIONS "BOTLIB"
)

# Build targets

# Emscripten static game modules
if(EMSCRIPTEN)
    # cgame
    add_library(cgame_et_static STATIC ${ET_CGAME_SRC})
    target_include_directories(
        cgame_et_static
        PRIVATE src/cgame src/game src/ui src/qcommon
    )
    target_compile_options(
        cgame_et_static
        PRIVATE
            ${ET_BASE_CFLAGS}
            -include
            ${CMAKE_CURRENT_SOURCE_DIR}/cgame_wasm_prefix.h
    )
    target_compile_definitions(cgame_et_static PRIVATE CGAMEDLL CGAME)

    # qagame
    add_library(qagame_et_static STATIC ${ET_GAME_SRC})
    target_include_directories(
        qagame_et_static
        PRIVATE src/game src/qcommon src/botlib src/botai
    )
    target_compile_options(
        qagame_et_static
        PRIVATE
            ${ET_BASE_CFLAGS}
            -include
            ${CMAKE_CURRENT_SOURCE_DIR}/qagame_wasm_prefix.h
    )
    target_compile_definitions(qagame_et_static PRIVATE GAMEDLL QAGAME)
    # Botlib sources in qagame need BOTLIB define
    file(GLOB _ET_BOTLIB_STATIC_SOURCES src/botlib/*.c)
    set_source_files_properties(
        ${_ET_BOTLIB_STATIC_SOURCES}
        TARGET_DIRECTORY qagame_et_static
        PROPERTIES COMPILE_DEFINITIONS "BOTLIB"
    )

    # ui
    add_library(ui_et_static STATIC ${ET_UI_SRC})
    target_include_directories(ui_et_static PRIVATE src/ui src/game src/qcommon)
    target_compile_options(
        ui_et_static
        PRIVATE
            ${ET_BASE_CFLAGS}
            -include
            ${CMAKE_CURRENT_SOURCE_DIR}/ui_wasm_prefix.h
    )
    target_compile_definitions(ui_et_static PRIVATE UI)
endif()

# Renderer OpenGL 1 (shared library)
if(ET_BUILD_CLIENT AND USE_RENDERER_DLOPEN)
    add_library(
        renderer_et_opengl1
        SHARED
        ${ET_RENDERER_SRC}
        ${ET_RENDERER_COMMON_SRC}
    )
    set_target_properties(
        renderer_et_opengl1
        PROPERTIES
            OUTPUT_NAME "renderer_et_opengl1_${SHLIBNAME}"
            PREFIX ""
            SUFFIX ""
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
    target_include_directories(
        renderer_et_opengl1
        PRIVATE
            src/renderer
            src/sdl
            src/qcommon
            src/game
            src/client
            src/server
            src/sys
    )
    target_compile_options(
        renderer_et_opengl1
        PRIVATE ${ET_BASE_CFLAGS} ${SHLIBCFLAGS}
    )
    target_compile_definitions(renderer_et_opengl1 PRIVATE USE_RENDERER_DLOPEN)
    if(USE_INTERNAL_JPEG)
        target_compile_definitions(
            renderer_et_opengl1
            PRIVATE USE_INTERNAL_JPEG
        )
    endif()

    target_include_directories(renderer_et_opengl1 PRIVATE ${SDL2_INCLUDE_DIR})
    target_link_libraries(renderer_et_opengl1 PRIVATE ${SDL2_LIBRARY})

    if(USE_INTERNAL_JPEG)
        target_link_libraries(renderer_et_opengl1 PRIVATE internal_jpeg)
    else()
        target_link_libraries(renderer_et_opengl1 PRIVATE ${JPEG_LIBRARIES})
        target_include_directories(
            renderer_et_opengl1
            PRIVATE ${JPEG_INCLUDE_DIRS}
        )
    endif()

    if(USE_INTERNAL_ZLIB)
        target_link_libraries(renderer_et_opengl1 PRIVATE internal_zlib)
    else()
        target_link_libraries(renderer_et_opengl1 PRIVATE ZLIB::ZLIB)
    endif()

    find_package(OpenGL REQUIRED)
    target_link_libraries(renderer_et_opengl1 PRIVATE OpenGL::GL)

    if(NOT WIN32)
        target_link_libraries(renderer_et_opengl1 PRIVATE m)
    endif()
    if(PLATFORM STREQUAL "linux")
        target_link_libraries(renderer_et_opengl1 PRIVATE dl pthread)
    endif()
endif()

# Client executable
# ET does not support renderer dlopen - renderer is always linked in
if(ET_BUILD_CLIENT)
    set(ET_CLIENT_ALL_SRC
        ${ET_CLIENT_SRC}
        ${ET_SPLINES_SRC}
        ${ET_RENDERER_SRC}
        ${ET_RENDERER_COMMON_SRC}
    )

    # Emscripten: use download stubs instead of curl
    if(EMSCRIPTEN)
        list(REMOVE_ITEM ET_CLIENT_ALL_SRC src/qcommon/dl_main_curl.c)
        list(APPEND ET_CLIENT_ALL_SRC src/qcommon/dl_main_stubs.c)
    endif()

    add_executable(iowolfet ${ET_CLIENT_ALL_SRC})
    set_target_properties(
        iowolfet
        PROPERTIES
            OUTPUT_NAME "iowolfet.${ARCH}"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
    target_include_directories(
        iowolfet
        PRIVATE
            src/client
            src/server
            src/renderer
            src/qcommon
            src/game
            src/sdl
            src/sys
            src/botlib
            src/splines
            src/ui
    )
    if(NOT EMSCRIPTEN)
        target_include_directories(iowolfet PRIVATE ${SDL2_INCLUDE_DIR})
    endif()
    target_compile_options(iowolfet PRIVATE ${ET_BASE_CFLAGS})

    add_wolf_definitions(iowolfet)
    link_wolf_libs(iowolfet)

    if(NOT EMSCRIPTEN)
        target_link_libraries(iowolfet PRIVATE ${SDL2_LIBRARY})
    endif()

    if(USE_INTERNAL_JPEG)
        target_link_libraries(iowolfet PRIVATE internal_jpeg)
    else()
        target_link_libraries(iowolfet PRIVATE ${JPEG_LIBRARIES})
        target_include_directories(iowolfet PRIVATE ${JPEG_INCLUDE_DIRS})
    endif()
    if(NOT EMSCRIPTEN)
        find_package(OpenGL REQUIRED)
        target_link_libraries(iowolfet PRIVATE OpenGL::GL)
    endif()

    if(USE_FREETYPE)
        if(USE_INTERNAL_FREETYPE)
            target_link_libraries(iowolfet PRIVATE internal_freetype)
        else()
            target_link_libraries(iowolfet PRIVATE ${FREETYPE_LIBRARIES})
            target_include_directories(
                iowolfet
                PRIVATE ${FREETYPE_INCLUDE_DIRS}
            )
        endif()
    endif()

    # CURL for downloads
    if(CURL_FOUND)
        target_link_libraries(iowolfet PRIVATE ${CURL_LIBRARIES})
        target_include_directories(iowolfet PRIVATE ${CURL_INCLUDE_DIRS})
    endif()

    if(EMSCRIPTEN)
        target_compile_options(iowolfet PRIVATE -sUSE_SDL=2)
        target_link_options(
            iowolfet
            PRIVATE
                -sUSE_SDL=2
                -sLEGACY_GL_EMULATION=1
                -sGL_SUPPORT_AUTOMATIC_ENABLE_EXTENSIONS=1
                -sALLOW_MEMORY_GROWTH=1
                -sINITIAL_MEMORY=234881024
                "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
                -sSTACK_SIZE=2097152
                -sALLOW_TABLE_GROWTH=1
                "SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/src/sys/shell_emscripten.html"
                "SHELL:--js-library ${CMAKE_CURRENT_SOURCE_DIR}/src/sys/net_master_ws.js"
        )
        set_target_properties(iowolfet PROPERTIES SUFFIX ".html")
        target_link_libraries(
            iowolfet
            PRIVATE cgame_et_static qagame_et_static ui_et_static
        )

        # Preload game data if GAME_DATA_DIR is set
        if(GAME_DATA_DIR)
            file(GLOB _et_paks "${GAME_DATA_DIR}/*.pk3")
            foreach(_pak ${_et_paks})
                get_filename_component(_pakname "${_pak}" NAME)
                target_link_options(
                    iowolfet
                    PRIVATE "SHELL:--preload-file ${_pak}@etmain/${_pakname}"
                )
            endforeach()
        endif()
    elseif(PLATFORM STREQUAL "linux")
        target_link_libraries(iowolfet PRIVATE dl m pthread)
    elseif(PLATFORM STREQUAL "freebsd")
        target_link_libraries(iowolfet PRIVATE m pthread)
    elseif(PLATFORM STREQUAL "darwin")
        target_link_libraries(
            iowolfet
            PRIVATE "-framework IOKit" "-framework Cocoa"
        )
    elseif(WIN32)
        target_link_libraries(
            iowolfet
            PRIVATE ws2_32 winmm psapi gdi32 ole32
        )
    endif()
endif()

# Dedicated server
if(ET_BUILD_SERVER)
    add_executable(iowolfetded ${ET_DEDICATED_SRC})
    set_target_properties(
        iowolfetded
        PROPERTIES
            OUTPUT_NAME "iowolfetded.${ARCH}"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
    target_include_directories(
        iowolfetded
        PRIVATE src/server src/qcommon src/game src/sys src/botlib src/null
    )
    target_compile_options(iowolfetded PRIVATE ${ET_BASE_CFLAGS})
    target_compile_definitions(iowolfetded PRIVATE DEDICATED)

    link_wolf_libs(iowolfetded)

    if(EMSCRIPTEN)
        target_compile_options(iowolfetded PRIVATE -sUSE_SDL=2)
        target_link_options(iowolfetded PRIVATE
            -sALLOW_MEMORY_GROWTH=1
            -sINITIAL_MEMORY=134217728
            -sSTACK_SIZE=2097152
            -sALLOW_TABLE_GROWTH=1
            -sENVIRONMENT=node
            -sEXIT_RUNTIME=1
            -lnodefs.js
            "--pre-js=${CMAKE_CURRENT_SOURCE_DIR}/src/sys/ded_node_prejs.js"
                "SHELL:--js-library ${CMAKE_CURRENT_SOURCE_DIR}/src/sys/net_master_ws.js"
        )
        set_target_properties(iowolfetded PROPERTIES SUFFIX ".js")
        target_link_libraries(iowolfetded PRIVATE qagame_et_static)
    elseif(PLATFORM STREQUAL "linux")
        target_link_libraries(iowolfetded PRIVATE dl m pthread)
    elseif(PLATFORM STREQUAL "freebsd")
        target_link_libraries(iowolfetded PRIVATE m pthread)
    elseif(PLATFORM STREQUAL "darwin")
        target_link_libraries(iowolfetded PRIVATE "-framework Cocoa")
    elseif(WIN32)
        target_link_libraries(iowolfetded PRIVATE ws2_32 winmm psapi)
    endif()
endif()

# Game modules (shared libraries)
if(ET_BUILD_GAME_SO)
    # cgame
    add_library(cgame_et SHARED ${ET_CGAME_SRC})
    set_target_properties(
        cgame_et
        PROPERTIES
            OUTPUT_NAME "cgame${ET_GAMEMOD_SHLIBNAME}"
            PREFIX ""
            SUFFIX ""
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/etmain"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/etmain"
    )
    target_include_directories(
        cgame_et
        PRIVATE src/cgame src/game src/ui src/qcommon
    )
    target_compile_options(cgame_et PRIVATE ${ET_BASE_CFLAGS} ${SHLIBCFLAGS})
    target_compile_definitions(cgame_et PRIVATE CGAMEDLL CGAME)
    if(NOT WIN32)
        target_link_libraries(cgame_et PRIVATE m)
    endif()

    # qagame
    add_library(qagame_et SHARED ${ET_GAME_SRC})
    set_target_properties(
        qagame_et
        PROPERTIES
            OUTPUT_NAME "qagame${ET_GAMEMOD_SHLIBNAME}"
            PREFIX ""
            SUFFIX ""
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/etmain"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/etmain"
    )
    target_include_directories(
        qagame_et
        PRIVATE src/game src/qcommon src/botlib src/botai
    )
    target_compile_options(qagame_et PRIVATE ${ET_BASE_CFLAGS} ${SHLIBCFLAGS})
    target_compile_definitions(qagame_et PRIVATE GAMEDLL QAGAME)
    if(NOT WIN32)
        target_link_libraries(qagame_et PRIVATE m)
    endif()

    # ui
    add_library(ui_et SHARED ${ET_UI_SRC})
    set_target_properties(
        ui_et
        PROPERTIES
            OUTPUT_NAME "ui${ET_GAMEMOD_SHLIBNAME}"
            PREFIX ""
            SUFFIX ""
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/etmain"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/etmain"
    )
    target_include_directories(ui_et PRIVATE src/ui src/game src/qcommon)
    target_compile_options(ui_et PRIVATE ${ET_BASE_CFLAGS} ${SHLIBCFLAGS})
    target_compile_definitions(ui_et PRIVATE UI)
    if(NOT WIN32)
        target_link_libraries(ui_et PRIVATE m)
    endif()
endif()

# Install rules
if(ET_BUILD_CLIENT)
    install(TARGETS iowolfet RUNTIME DESTINATION bin)
    if(USE_RENDERER_DLOPEN)
        install(TARGETS renderer_et_opengl1 LIBRARY DESTINATION bin)
    endif()
endif()

if(ET_BUILD_SERVER)
    install(TARGETS iowolfetded RUNTIME DESTINATION bin)
endif()

if(ET_BUILD_GAME_SO)
    install(TARGETS cgame_et qagame_et ui_et LIBRARY DESTINATION bin/etmain)
endif()
