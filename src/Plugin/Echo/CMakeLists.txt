dpf_add_plugin(ZynEcho
    TARGETS clap lv2 vst2 vst3
    FILES_DSP
        Echo.cpp
)

target_include_directories(ZynEcho PUBLIC ".")

target_link_libraries(ZynEcho PRIVATE zynaddsubfx_core)

dpf__determine_vst3_package_architecture(vst3_arch)
install(TARGETS ZynEcho-lv2 LIBRARY DESTINATION ${PluginLibDir}/lv2/ZynEcho.lv2/)
install(TARGETS ZynEcho-vst2 LIBRARY DESTINATION ${PluginLibDir}/vst/)
install(TARGETS ZynEcho-vst3 LIBRARY DESTINATION ${PluginLibDir}/vst3/ZynEcho.vst3/Contents/${vst3_arch}/)
install(TARGETS ZynEcho-clap LIBRARY DESTINATION ${PluginLibDir}/clap/)

install(FILES
	${CMAKE_BINARY_DIR}/bin/ZynEcho.lv2/manifest.ttl
	${CMAKE_BINARY_DIR}/bin/ZynEcho.lv2/presets.ttl
	${CMAKE_BINARY_DIR}/bin/ZynEcho.lv2/ZynEcho_dsp.ttl
	DESTINATION ${PluginLibDir}/lv2/ZynEcho.lv2/)
