
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/DPF/distrho .)

add_library(ZynDistortion_lv2 SHARED ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp Distortion.cpp)
add_library(ZynDistortion_vst SHARED ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp Distortion.cpp)

set_target_properties(ZynDistortion_lv2 PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_LV2")
set_target_properties(ZynDistortion_vst PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_VST")

target_link_libraries(ZynDistortion_lv2 zynaddsubfx_core ${OS_LIBRARIES})
target_link_libraries(ZynDistortion_vst zynaddsubfx_core ${OS_LIBRARIES})
