
include_directories(${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/DPF/distrho .
    ${CMAKE_SOURCE_DIR}/DPF/dgl
    ${CMAKE_SOURCE_DIR}/DPF/dgl/src)

if(NtkGui)

# UI Enabled using NTK: embed + external
add_library(ZynAddSubFX_lv2 SHARED
    ${CMAKE_SOURCE_DIR}/src/globals.cpp
    ${CMAKE_SOURCE_DIR}/src/UI/ConnectionDummy.cpp
    ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp
    ZynAddSubFX.cpp)

add_library(ZynAddSubFX_lv2_ui SHARED
    ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoUIMain.cpp
    ZynAddSubFX-UI.cpp)

add_library(ZynAddSubFX_vst SHARED
    ${CMAKE_SOURCE_DIR}/src/globals.cpp
    ${CMAKE_SOURCE_DIR}/src/UI/ConnectionDummy.cpp
    ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp
    ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoUIMain.cpp
    ZynAddSubFX.cpp
    ZynAddSubFX-UI.cpp)

# TODO: Make this also check for Linux or MacOS
elseif(FltkGui)

# UI Enabled using FLTK: external only
add_library(ZynAddSubFX_lv2 SHARED
    ${CMAKE_SOURCE_DIR}/src/globals.cpp
    ${CMAKE_SOURCE_DIR}/src/UI/ConnectionDummy.cpp
    ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp
    ZynAddSubFX.cpp)

add_library(ZynAddSubFX_lv2_ui SHARED
    ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoUIMain.cpp
    ZynAddSubFX-UI.cpp)

add_library(ZynAddSubFX_vst SHARED
    ${CMAKE_SOURCE_DIR}/src/globals.cpp
    ${CMAKE_SOURCE_DIR}/src/UI/ConnectionDummy.cpp
    ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp
    ZynAddSubFX.cpp)

elseif(ZestGui)

# UI Enabled using Zest: internal only
if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
add_library(ZynAddSubFX_lv2 SHARED
    ${CMAKE_SOURCE_DIR}/src/globals.cpp
    ${CMAKE_SOURCE_DIR}/src/UI/ConnectionDummy.cpp
    ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp
    ZynAddSubFX.cpp)

add_library(ZynAddSubFX_lv2_ui SHARED
    ${CMAKE_SOURCE_DIR}/DPF/dgl/src/Geometry.cpp
    ${CMAKE_SOURCE_DIR}/DPF/dgl/src/Widget.cpp
    ${CMAKE_SOURCE_DIR}/DPF/dgl/src/Window.cpp
    ${CMAKE_SOURCE_DIR}/DPF/dgl/src/Application.cpp
    ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoUIMain.cpp
    ZynAddSubFX-UI-Zest.cpp)
endif()

add_library(ZynAddSubFX_vst SHARED
    ${CMAKE_SOURCE_DIR}/src/globals.cpp
    ${CMAKE_SOURCE_DIR}/src/UI/ConnectionDummy.cpp
    ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp
    ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoUIMain.cpp
    ${CMAKE_SOURCE_DIR}/DPF/dgl/src/Geometry.cpp
    ${CMAKE_SOURCE_DIR}/DPF/dgl/src/Widget.cpp
    ${CMAKE_SOURCE_DIR}/DPF/dgl/src/Window.cpp
    ${CMAKE_SOURCE_DIR}/DPF/dgl/src/Application.cpp
    ZynAddSubFX.cpp
    ZynAddSubFX-UI-Zest.cpp)

else()

# UI Disabled
add_library(ZynAddSubFX_lv2 SHARED
    ${CMAKE_SOURCE_DIR}/src/globals.cpp
    ${CMAKE_SOURCE_DIR}/src/UI/ConnectionDummy.cpp
    ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp
    ZynAddSubFX.cpp)

add_library(ZynAddSubFX_vst SHARED
    ${CMAKE_SOURCE_DIR}/src/globals.cpp
    ${CMAKE_SOURCE_DIR}/src/UI/ConnectionDummy.cpp
    ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp
    ZynAddSubFX.cpp)

endif()

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set_target_properties(ZynAddSubFX_lv2 PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_LV2")
    set_target_properties(ZynAddSubFX_lv2 PROPERTIES LIBRARY_OUTPUT_DIRECTORY "lv2")
    set_target_properties(ZynAddSubFX_lv2 PROPERTIES OUTPUT_NAME "ZynAddSubFX")
    set_target_properties(ZynAddSubFX_lv2 PROPERTIES PREFIX "")
endif()

set_target_properties(ZynAddSubFX_vst PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_VST")
set_target_properties(ZynAddSubFX_vst PROPERTIES LIBRARY_OUTPUT_DIRECTORY "vst")
set_target_properties(ZynAddSubFX_vst PROPERTIES OUTPUT_NAME "ZynAddSubFX")
set_target_properties(ZynAddSubFX_vst PROPERTIES PREFIX "")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(PLATFORM_LIBRARIES ws2_32
            winmm
            glu32 
            gdi32
            opengl32
            wsock32
            "-static" iphlpapi
            "-static" winpthread)
elseif(ZestGui)
    if(APPLE)
        set(PLATFORM_LIBRARIES)
    else()
        if(HAVE_LIBRT)
            set(PLATFORM_LIBRARIES X11 GL rt)
        else()
            set(PLATFORM_LIBRARIES X11 GL)
        endif()
    endif()
elseif(NtkGui OR FltkGui)
    if(HAVE_LIBRT)
        set(PLATFORM_LIBRARIES X11 rt)
    else()
        set(PLATFORM_LIBRARIES X11)
    endif()
else()
    if(HAVE_LIBRT)
        set(PLATFORM_LIBRARIES rt)
    endif()
endif()

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    target_link_libraries(ZynAddSubFX_lv2 zynaddsubfx_core ${OS_LIBRARIES} ${LIBLO_LIBRARIES}
        ${PLATFORM_LIBRARIES})
endif()
if(APPLE)
    target_link_libraries(ZynAddSubFX_vst zynaddsubfx_core ${OS_LIBRARIES} ${LIBLO_LIBRARIES}
        ${PLATFORM_LIBRARIES} "-framework Cocoa" "-framework openGL" "-Wl,-exported_symbol,_VSTPluginMain")
else()
    target_link_libraries(ZynAddSubFX_vst zynaddsubfx_core ${OS_LIBRARIES} ${LIBLO_LIBRARIES}
        ${PLATFORM_LIBRARIES})
endif()
if(ZestGui AND NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    if(APPLE)
        target_link_libraries(ZynAddSubFX_lv2_ui "-framework Cocoa" "-framework openGL" "-Wl,-exported_symbol,_lv2ui_descriptor")
        target_link_libraries(ZynAddSubFX_lv2 "-Wl,-exported_symbol,_lv2_descriptor" "-Wl,-exported_symbol,_lv2_generate_ttl")
    else()
        target_link_libraries(ZynAddSubFX_lv2_ui X11 GL)
    endif()
endif()

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    install(TARGETS ZynAddSubFX_lv2 LIBRARY DESTINATION ${PluginLibDir}/lv2/ZynAddSubFX.lv2/)
    install(TARGETS ZynAddSubFX_vst LIBRARY DESTINATION ${PluginLibDir}/vst/)

    add_custom_command(TARGET ZynAddSubFX_lv2 POST_BUILD
        COMMAND ../../lv2-ttl-generator $<TARGET_FILE:ZynAddSubFX_lv2>
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lv2)

    add_dependencies(ZynAddSubFX_lv2 lv2-ttl-generator)
endif()

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/lv2/manifest.ttl
    ${CMAKE_CURRENT_BINARY_DIR}/lv2/presets.ttl
    ${CMAKE_CURRENT_BINARY_DIR}/lv2/ZynAddSubFX.ttl
    DESTINATION ${PluginLibDir}/lv2/ZynAddSubFX.lv2/)

if((NtkGui OR FltkGui OR ZestGui) AND NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
set_target_properties(ZynAddSubFX_lv2_ui PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_LV2")
set_target_properties(ZynAddSubFX_lv2_ui PROPERTIES LIBRARY_OUTPUT_DIRECTORY "lv2")
set_target_properties(ZynAddSubFX_lv2_ui PROPERTIES OUTPUT_NAME "ZynAddSubFX_ui")
set_target_properties(ZynAddSubFX_lv2_ui PROPERTIES PREFIX "")

install(TARGETS ZynAddSubFX_lv2_ui LIBRARY DESTINATION ${PluginLibDir}/lv2/ZynAddSubFX.lv2/)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/lv2/ZynAddSubFX_ui.ttl
        DESTINATION ${PluginLibDir}/lv2/ZynAddSubFX.lv2/)
endif()
