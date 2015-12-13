
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/DPF/distrho .)

add_library(ZynReverb_lv2 SHARED ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp Reverb.cpp)
add_library(ZynReverb_vst SHARED ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp Reverb.cpp)

set_target_properties(ZynReverb_lv2 PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_LV2")
set_target_properties(ZynReverb_vst PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_VST")

target_link_libraries(ZynReverb_lv2 zynaddsubfx_core ${OS_LIBRARIES})
target_link_libraries(ZynReverb_vst zynaddsubfx_core ${OS_LIBRARIES})
