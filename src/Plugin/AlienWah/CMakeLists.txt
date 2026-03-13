dpf_add_plugin(ZynAlienWah
    TARGETS clap lv2 vst2 vst3
    FILES_DSP
        AlienWah.cpp
)

include_directories(SYSTEM ${CMAKE_CURRENT_BINARY_DIR} .)

target_link_libraries(ZynAlienWah PRIVATE zynaddsubfx_core)

dpf__determine_vst3_package_architecture(vst3_arch)
install(TARGETS ZynAlienWah-lv2 LIBRARY DESTINATION ${PluginLibDir}/lv2/ZynAlienWah.lv2/)
install(TARGETS ZynAlienWah-vst2 LIBRARY DESTINATION ${PluginLibDir}/vst/)
install(TARGETS ZynAlienWah-vst3 LIBRARY DESTINATION ${PluginLibDir}/vst3/ZynAlienWah.vst3/Contents/${vst3_arch}/)
install(TARGETS ZynAlienWah-clap LIBRARY DESTINATION ${PluginLibDir}/clap/)

install(FILES
	${CMAKE_BINARY_DIR}/bin/ZynAlienWah.lv2/manifest.ttl
	${CMAKE_BINARY_DIR}/bin/ZynAlienWah.lv2/presets.ttl
	${CMAKE_BINARY_DIR}/bin/ZynAlienWah.lv2/ZynAlienWah_dsp.ttl
	DESTINATION ${PluginLibDir}/lv2/ZynAlienWah.lv2/)
