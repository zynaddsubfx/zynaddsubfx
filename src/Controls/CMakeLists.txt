set(zynaddsubfx_controls_SRCS
	Control.C
	DelayCtl.C
)

add_library(zynaddsubfx_controls STATIC
	${zynaddsubfx_controls_SRCS} 
	)
