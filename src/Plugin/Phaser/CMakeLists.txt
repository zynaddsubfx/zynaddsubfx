dpf_add_plugin(ZynPhaser
    TARGETS clap lv2 vst2 vst3
    FILES_DSP
        Phaser.cpp
)

include_directories(SYSTEM ${CMAKE_CURRENT_BINARY_DIR} .)

target_link_libraries(ZynPhaser PRIVATE zynaddsubfx_core)

dpf__determine_vst3_package_architecture(vst3_arch)
install(TARGETS ZynPhaser-lv2 LIBRARY DESTINATION ${PluginLibDir}/lv2/ZynPhaser.lv2/)
install(TARGETS ZynPhaser-vst2 LIBRARY DESTINATION ${PluginLibDir}/vst/)
install(TARGETS ZynPhaser-vst3 LIBRARY DESTINATION ${PluginLibDir}/vst3/ZynPhaser.vst3/Contents/${vst3_arch}/)
install(TARGETS ZynPhaser-clap LIBRARY DESTINATION ${PluginLibDir}/clap/)

install(FILES
	${CMAKE_BINARY_DIR}/bin/ZynPhaser.lv2/manifest.ttl
	${CMAKE_BINARY_DIR}/bin/ZynPhaser.lv2/presets.ttl
	${CMAKE_BINARY_DIR}/bin/ZynPhaser.lv2/ZynPhaser_dsp.ttl
	DESTINATION ${PluginLibDir}/lv2/ZynPhaser.lv2/)
