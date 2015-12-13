
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/DPF/distrho .)

add_library(ZynAlienWah_lv2 SHARED ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp AlienWah.cpp)
add_library(ZynAlienWah_vst SHARED ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp AlienWah.cpp)

set_target_properties(ZynAlienWah_lv2 PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_LV2")
set_target_properties(ZynAlienWah_lv2 PROPERTIES LIBRARY_OUTPUT_DIRECTORY "lv2")
set_target_properties(ZynAlienWah_lv2 PROPERTIES OUTPUT_NAME "ZynAlienWah")
set_target_properties(ZynAlienWah_lv2 PROPERTIES PREFIX "")

set_target_properties(ZynAlienWah_vst PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_VST")
set_target_properties(ZynAlienWah_vst PROPERTIES LIBRARY_OUTPUT_DIRECTORY "vst")
set_target_properties(ZynAlienWah_vst PROPERTIES OUTPUT_NAME "ZynAlienWah")
set_target_properties(ZynAlienWah_vst PROPERTIES PREFIX "")

target_link_libraries(ZynAlienWah_lv2 zynaddsubfx_core ${OS_LIBRARIES})
target_link_libraries(ZynAlienWah_vst zynaddsubfx_core ${OS_LIBRARIES})

install(TARGETS ZynAlienWah_lv2 LIBRARY DESTINATION lib/lv2/ZynAlienWah.lv2/)
install(TARGETS ZynAlienWah_vst LIBRARY DESTINATION lib/vst/)
