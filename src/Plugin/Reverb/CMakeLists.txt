dpf_add_plugin(ZynReverb
    TARGETS clap lv2 vst2 vst3
    FILES_DSP
        Reverb.cpp
)

include_directories(SYSTEM ${CMAKE_CURRENT_BINARY_DIR} .)

target_link_libraries(ZynReverb PRIVATE zynaddsubfx_core)

dpf__determine_vst3_package_architecture(vst3_arch)
install(TARGETS ZynReverb-lv2 LIBRARY DESTINATION ${PluginLibDir}/lv2/ZynReverb.lv2/)
install(TARGETS ZynReverb-vst2 LIBRARY DESTINATION ${PluginLibDir}/vst/)
install(TARGETS ZynReverb-vst3 LIBRARY DESTINATION ${PluginLibDir}/vst3/ZynReverb.vst3/Contents/${vst3_arch}/)
install(TARGETS ZynReverb-clap LIBRARY DESTINATION ${PluginLibDir}/clap/)

install(FILES
	${CMAKE_BINARY_DIR}/bin/ZynReverb.lv2/manifest.ttl
	${CMAKE_BINARY_DIR}/bin/ZynReverb.lv2/presets.ttl
	${CMAKE_BINARY_DIR}/bin/ZynReverb.lv2/ZynReverb_dsp.ttl
	DESTINATION ${PluginLibDir}/lv2/ZynReverb.lv2/)
