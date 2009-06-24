set(zynaddsubfx_effect_SRCS
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

add_library(zynaddsubfx_effect STATIC
	${zynaddsubfx_effect_SRCS} 
	)

target_link_libraries(zynaddsubfx_effect ${ASOUND_LIBRARY})
