set(zynaddsubfxgui_effect_SRCS
	Alienwah.C
	Chorus.C
	Distorsion.C
	DynamicFilter.C
	Echo.C
	Effect.C
	EffectLFO.C
	EffectMgr.C
	EQ.C
	Phaser.C
	Reverb.C
)

add_library(zynaddsubfxgui_effect STATIC
	${zynaddsubfxgui_effect_SRCS} 
	)

target_link_libraries(zynaddsubfxgui_effect ${ASOUND_LIBRARY})
