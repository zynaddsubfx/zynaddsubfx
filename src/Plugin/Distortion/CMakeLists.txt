dpf_add_plugin(ZynDistortion
    TARGETS clap lv2 vst2 vst3
    FILES_DSP
        Distortion.cpp
)

target_include_directories(ZynDistortion PUBLIC ".")

target_link_libraries(ZynDistortion PRIVATE zynaddsubfx_core)

dpf__determine_vst3_package_architecture(vst3_arch)
install(TARGETS ZynDistortion-lv2 LIBRARY DESTINATION ${PluginLibDir}/lv2/ZynDistortion.lv2/)
install(TARGETS ZynDistortion-vst2 LIBRARY DESTINATION ${PluginLibDir}/vst/)
install(TARGETS ZynDistortion-vst3 LIBRARY DESTINATION ${PluginLibDir}/vst3/ZynDistortion.vst3/Contents/${vst3_arch}/)
install(TARGETS ZynDistortion-clap LIBRARY DESTINATION ${PluginLibDir}/clap/)

install(FILES
	${CMAKE_BINARY_DIR}/bin/ZynDistortion.lv2/manifest.ttl
	${CMAKE_BINARY_DIR}/bin/ZynDistortion.lv2/presets.ttl
	${CMAKE_BINARY_DIR}/bin/ZynDistortion.lv2/ZynDistortion_dsp.ttl
	DESTINATION ${PluginLibDir}/lv2/ZynDistortion.lv2/)
