
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/DPF/distrho .)

add_library(ZynAlienWah_lv2 SHARED ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp AlienWah.cpp)
add_library(ZynAlienWah_vst SHARED ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp AlienWah.cpp)

set_target_properties(ZynAlienWah_lv2 PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_LV2")
set_target_properties(ZynAlienWah_vst PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_VST")

target_link_libraries(ZynAlienWah_lv2 zynaddsubfx_core ${OS_LIBRARIES})
target_link_libraries(ZynAlienWah_vst zynaddsubfx_core ${OS_LIBRARIES})
