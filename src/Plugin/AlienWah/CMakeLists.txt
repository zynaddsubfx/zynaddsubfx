
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

if(APPLE)
    target_link_libraries(ZynAlienWah_lv2 zynaddsubfx_core ${OS_LIBRARIES} "-Wl,-exported_symbol,_lv2_descriptor" "-Wl,-exported_symbol,_lv2_generate_ttl")
    target_link_libraries(ZynAlienWah_vst zynaddsubfx_core ${OS_LIBRARIES} "-Wl,-exported_symbol,_VSTPluginMain")
else()
    target_link_libraries(ZynAlienWah_lv2 zynaddsubfx_core ${OS_LIBRARIES})
    target_link_libraries(ZynAlienWah_vst zynaddsubfx_core ${OS_LIBRARIES})
endif()

install(TARGETS ZynAlienWah_lv2 LIBRARY DESTINATION ${PluginLibDir}/lv2/ZynAlienWah.lv2/)
install(TARGETS ZynAlienWah_vst LIBRARY DESTINATION ${PluginLibDir}/vst/)

add_custom_command(TARGET ZynAlienWah_lv2 POST_BUILD
    COMMAND ../../lv2-ttl-generator $<TARGET_FILE:ZynAlienWah_lv2>
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lv2)

add_dependencies(ZynAlienWah_lv2 lv2-ttl-generator)

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/lv2/manifest.ttl
	${CMAKE_CURRENT_BINARY_DIR}/lv2/presets.ttl
	${CMAKE_CURRENT_BINARY_DIR}/lv2/ZynAlienWah.ttl
	DESTINATION ${PluginLibDir}/lv2/ZynAlienWah.lv2/)
