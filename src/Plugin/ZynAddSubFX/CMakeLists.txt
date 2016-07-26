
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/DPF/distrho .)

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

set_target_properties(ZynAddSubFX_lv2 PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_LV2")
set_target_properties(ZynAddSubFX_lv2 PROPERTIES LIBRARY_OUTPUT_DIRECTORY "lv2")
set_target_properties(ZynAddSubFX_lv2 PROPERTIES OUTPUT_NAME "ZynAddSubFX")
set_target_properties(ZynAddSubFX_lv2 PROPERTIES PREFIX "")

set_target_properties(ZynAddSubFX_vst PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_VST")
set_target_properties(ZynAddSubFX_vst PROPERTIES LIBRARY_OUTPUT_DIRECTORY "vst")
set_target_properties(ZynAddSubFX_vst PROPERTIES OUTPUT_NAME "ZynAddSubFX")
set_target_properties(ZynAddSubFX_vst PROPERTIES PREFIX "")

target_link_libraries(ZynAddSubFX_lv2 zynaddsubfx_core ${OS_LIBRARIES} ${LIBLO_LIBRARIES}
    ${PLATFORM_LIBRARIES})
target_link_libraries(ZynAddSubFX_vst zynaddsubfx_core ${OS_LIBRARIES} ${LIBLO_LIBRARIES}
    ${PLATFORM_LIBRARIES})

install(TARGETS ZynAddSubFX_lv2 LIBRARY DESTINATION ${PluginLibDir}/lv2/ZynAddSubFX.lv2/)
install(TARGETS ZynAddSubFX_vst LIBRARY DESTINATION ${PluginLibDir}/vst/)

add_custom_command(TARGET ZynAddSubFX_lv2 POST_BUILD
    COMMAND ../../lv2-ttl-generator $<TARGET_FILE:ZynAddSubFX_lv2>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lv2)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/lv2/manifest.ttl
    ${CMAKE_CURRENT_BINARY_DIR}/lv2/presets.ttl
    ${CMAKE_CURRENT_BINARY_DIR}/lv2/ZynAddSubFX.ttl
    DESTINATION ${PluginLibDir}/lv2/ZynAddSubFX.lv2/)

if(NtkGui OR FltkGui)
set_target_properties(ZynAddSubFX_lv2_ui PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_LV2")
set_target_properties(ZynAddSubFX_lv2_ui PROPERTIES LIBRARY_OUTPUT_DIRECTORY "lv2")
set_target_properties(ZynAddSubFX_lv2_ui PROPERTIES OUTPUT_NAME "ZynAddSubFX_ui")
set_target_properties(ZynAddSubFX_lv2_ui PROPERTIES PREFIX "")

install(TARGETS ZynAddSubFX_lv2_ui LIBRARY DESTINATION ${PluginLibDir}/lv2/ZynAddSubFX.lv2/)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/lv2/ZynAddSubFX_ui.ttl
        DESTINATION ${PluginLibDir}/lv2/ZynAddSubFX.lv2/)
endif()
