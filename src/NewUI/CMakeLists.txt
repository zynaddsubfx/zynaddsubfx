
set(zynaddsubfxguilib_SRCS
	dial.cpp
	bankui.C
	masterui.C
	addnoteui.C
	unitbox.C
	lfobox.C
	envelopebox.C
	)

SET( zynaddsubfxguilib_MOC_HDRS
	masterui.h
	bankui.h
	addnoteui.h
	unitbox.h
	lfobox.h
	envelopebox.h
)

SET( zynaddsubfxguilib_UIS
	masterUI.ui
	addnoteui.ui
	lfobox.ui
	envelopebox.ui
)

qt4_wrap_ui(zynaddsubfxguilib_UIS_HDRS ${zynaddsubfxguilib_UIS} )
qt4_automoc(${zynaddsubfxguilib_SRCS})

include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_library(zynaddsubfxguilib STATIC
	${zynaddsubfxguilib_SRCS}
	${zynaddsubfxguilib_UIS}
	)
target_link_libraries(zynaddsubfxguilib)
