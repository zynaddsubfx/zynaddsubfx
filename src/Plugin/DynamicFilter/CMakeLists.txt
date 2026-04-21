dpf_add_plugin(ZynDynamicFilter
    TARGETS clap lv2 vst2 vst3
    FILES_DSP
        DynamicFilter.cpp
)

include_directories(SYSTEM ${CMAKE_CURRENT_BINARY_DIR} .)

target_link_libraries(ZynDynamicFilter PRIVATE zynaddsubfx_core)

dpf__determine_vst3_package_architecture(vst3_arch)
install(TARGETS ZynDynamicFilter-lv2 LIBRARY DESTINATION ${PluginLibDir}/lv2/ZynDynamicFilter.lv2/)
install(TARGETS ZynDynamicFilter-vst2 LIBRARY DESTINATION ${PluginLibDir}/vst/)
install(TARGETS ZynDynamicFilter-vst3 LIBRARY DESTINATION ${PluginLibDir}/vst3/ZynDynamicFilter.vst3/Contents/${vst3_arch}/)
install(TARGETS ZynDynamicFilter-clap LIBRARY DESTINATION ${PluginLibDir}/clap/)

install(FILES
	${CMAKE_BINARY_DIR}/bin/ZynDynamicFilter.lv2/manifest.ttl
	${CMAKE_BINARY_DIR}/bin/ZynDynamicFilter.lv2/presets.ttl
	${CMAKE_BINARY_DIR}/bin/ZynDynamicFilter.lv2/ZynDynamicFilter_dsp.ttl
	DESTINATION ${PluginLibDir}/lv2/ZynDynamicFilter.lv2/)
