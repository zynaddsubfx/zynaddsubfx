dpf_add_plugin(ZynChorus
    TARGETS clap lv2 vst2 vst3
    FILES_DSP
        Chorus.cpp
)

target_include_directories(ZynChorus PUBLIC ".")

target_link_libraries(ZynChorus PRIVATE zynaddsubfx_core)

dpf__determine_vst3_package_architecture(vst3_arch)
install(TARGETS ZynChorus-lv2 LIBRARY DESTINATION ${PluginLibDir}/lv2/ZynChorus.lv2/)
install(TARGETS ZynChorus-vst2 LIBRARY DESTINATION ${PluginLibDir}/vst/)
install(TARGETS ZynChorus-vst3 LIBRARY DESTINATION ${PluginLibDir}/vst3/ZynChorus.vst3/Contents/${vst3_arch}/)
install(TARGETS ZynChorus-clap LIBRARY DESTINATION ${PluginLibDir}/clap/)

install(FILES
	${CMAKE_BINARY_DIR}/bin/ZynChorus.lv2/manifest.ttl
	${CMAKE_BINARY_DIR}/bin/ZynChorus.lv2/presets.ttl
	${CMAKE_BINARY_DIR}/bin/ZynChorus.lv2/ZynChorus_dsp.ttl
	DESTINATION ${PluginLibDir}/lv2/ZynChorus.lv2/)
