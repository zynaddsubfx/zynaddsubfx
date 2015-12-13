
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/DPF/distrho .)

add_library(ZynEcho_lv2 SHARED ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp Echo.cpp)
add_library(ZynEcho_vst SHARED ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp Echo.cpp)

set_target_properties(ZynEcho_lv2 PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_LV2")
set_target_properties(ZynEcho_vst PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_VST")

target_link_libraries(ZynEcho_lv2 zynaddsubfx_core ${OS_LIBRARIES})
target_link_libraries(ZynEcho_vst zynaddsubfx_core ${OS_LIBRARIES})
