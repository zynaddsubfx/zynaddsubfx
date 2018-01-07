
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/DPF/distrho .)

add_library(ZynDistortion_lv2 SHARED ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp Distortion.cpp)
add_library(ZynDistortion_vst SHARED ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp Distortion.cpp)

set_target_properties(ZynDistortion_lv2 PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_LV2")
set_target_properties(ZynDistortion_lv2 PROPERTIES LIBRARY_OUTPUT_DIRECTORY "lv2")
set_target_properties(ZynDistortion_lv2 PROPERTIES OUTPUT_NAME "ZynDistortion")
set_target_properties(ZynDistortion_lv2 PROPERTIES PREFIX "")

set_target_properties(ZynDistortion_vst PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_VST")
set_target_properties(ZynDistortion_vst PROPERTIES LIBRARY_OUTPUT_DIRECTORY "vst")
set_target_properties(ZynDistortion_vst PROPERTIES OUTPUT_NAME "ZynDistortion")
set_target_properties(ZynDistortion_vst PROPERTIES PREFIX "")

if(APPLE)
    target_link_libraries(ZynDistortion_lv2 zynaddsubfx_core ${OS_LIBRARIES} "-Wl,-exported_symbol,_lv2_descriptor" "-Wl,-exported_symbol,_lv2_generate_ttl")
    target_link_libraries(ZynDistortion_vst zynaddsubfx_core ${OS_LIBRARIES} "-Wl,-exported_symbol,_VSTPluginMain")
else()
    target_link_libraries(ZynDistortion_lv2 zynaddsubfx_core ${OS_LIBRARIES})
    target_link_libraries(ZynDistortion_vst zynaddsubfx_core ${OS_LIBRARIES})
endif()

install(TARGETS ZynDistortion_lv2 LIBRARY DESTINATION ${PluginLibDir}/lv2/ZynDistortion.lv2/)
install(TARGETS ZynDistortion_vst LIBRARY DESTINATION ${PluginLibDir}/vst/)

add_custom_command(TARGET ZynDistortion_lv2 POST_BUILD
    COMMAND ../../lv2-ttl-generator $<TARGET_FILE:ZynDistortion_lv2>
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lv2)

add_dependencies(ZynDistortion_lv2 lv2-ttl-generator)

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/lv2/manifest.ttl
	${CMAKE_CURRENT_BINARY_DIR}/lv2/presets.ttl
	${CMAKE_CURRENT_BINARY_DIR}/lv2/ZynDistortion.ttl
    DESTINATION ${PluginLibDir}/lv2/ZynDistortion.lv2/)
