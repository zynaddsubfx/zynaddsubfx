# Common GUI stubs
# NOTICE:
# - Need to build into a dedicated static library.
# - Must be put AFTER "zynaddsubfx_core" in target_link_libraries(),
#   otherwise symbol lookup error will occur.
add_library(zynaddsubfx_gui_stubs STATIC
    ${CMAKE_SOURCE_DIR}/src/globals.cpp
    ${CMAKE_SOURCE_DIR}/src/UI/ConnectionDummy.cpp
)

if(NtkGui)

# UI Enabled using NTK: embed + external
dpf_add_plugin(ZynAddSubFX
    TARGETS clap lv2 vst2 vst3
    UI_TYPE external
    FILES_DSP
        ZynAddSubFX.cpp
    FILES_UI
        ZynAddSubFX-UI.cpp
)

# TODO: Make this also check for Linux or MacOS
elseif(FltkGui)

# UI Enabled using FLTK: external only
dpf_add_plugin(ZynAddSubFX
    TARGETS clap lv2 vst2 vst3
    UI_TYPE external
    FILES_DSP
        ZynAddSubFX.cpp
    FILES_UI
        ZynAddSubFX-UI.cpp
)

elseif(ZestGui)

# UI Enabled using Zest: internal only
dpf_add_plugin(ZynAddSubFX
    TARGETS clap lv2 vst2 vst3
    UI_TYPE opengl
    FILES_DSP
        ZynAddSubFX.cpp
    FILES_UI
        ZynAddSubFX-UI-Zest.cpp
)

else()

# UI Disabled
dpf_add_plugin(ZynAddSubFX
    TARGETS clap lv2 vst2 vst3
    FILES_DSP
        ZynAddSubFX.cpp
)
endif()

target_include_directories(ZynAddSubFX PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ".")

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

target_link_libraries(ZynAddSubFX-dsp PRIVATE zynaddsubfx_core zynaddsubfx_gui_stubs
    ${OS_LIBRARIES} ${LIBLO_LIBRARIES} ${PLATFORM_LIBRARIES})

# Linker diagnostic options
target_link_libraries(ZynAddSubFX PRIVATE -Wl,--no-undefined,--unresolved-symbols=report-all)

dpf__determine_vst3_package_architecture(vst3_arch)
install(TARGETS ZynAddSubFX-lv2 LIBRARY DESTINATION ${PluginLibDir}/lv2/ZynAddSubFX.lv2/)
install(TARGETS ZynAddSubFX-vst2 LIBRARY DESTINATION ${PluginLibDir}/vst/)
install(TARGETS ZynAddSubFX-vst3 LIBRARY DESTINATION ${PluginLibDir}/vst3/ZynAddSubFX.vst3/Contents/${vst3_arch}/)
install(TARGETS ZynAddSubFX-clap LIBRARY DESTINATION ${PluginLibDir}/clap/)

install(FILES
    ${CMAKE_BINARY_DIR}/bin/ZynAddSubFX.lv2/manifest.ttl
    ${CMAKE_BINARY_DIR}/bin/ZynAddSubFX.lv2/presets.ttl
    ${CMAKE_BINARY_DIR}/bin/ZynAddSubFX.lv2/ZynAddSubFX_dsp.ttl
    DESTINATION ${PluginLibDir}/lv2/ZynAddSubFX.lv2/)

if(NtkGui OR FltkGui OR ZestGui)

install(TARGETS ZynAddSubFX-lv2-ui LIBRARY DESTINATION ${PluginLibDir}/lv2/ZynAddSubFX.lv2/)

install(FILES
    ${CMAKE_BINARY_DIR}/bin/ZynAddSubFX.lv2/ZynAddSubFX_ui.ttl
    DESTINATION ${PluginLibDir}/lv2/ZynAddSubFX.lv2/)

endif()
