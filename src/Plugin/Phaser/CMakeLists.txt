
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/DPF/distrho .)

add_library(ZynPhaser_lv2 SHARED ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp Phaser.cpp)
add_library(ZynPhaser_vst SHARED ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp Phaser.cpp)

set_target_properties(ZynPhaser_lv2 PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_LV2")
set_target_properties(ZynPhaser_vst PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_VST")

target_link_libraries(ZynPhaser_lv2 zynaddsubfx_core ${OS_LIBRARIES})
target_link_libraries(ZynPhaser_vst zynaddsubfx_core ${OS_LIBRARIES})
