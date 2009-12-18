#Defaults:
#         - Wave Output (enabled with the record function)
#         - Null Output
#         - Null Output Running by default
#         - Managed with OutMgr
set(zynaddsubfx_nio_SRCS
    WavEngine.cpp
    NulEngine.cpp
    AudioOut.cpp
    OutMgr.cpp
    )

set(zynaddsubfx_nio_lib )

if(JackEnable)
    include_directories(${JACK_INCLUDE_DIR})
    list(APPEND zynaddsubfx_nio_SRCS JackEngine.cpp)
    list(APPEND zynaddsubfx_nio_lib ${JACK_LIBRARIES})
endif(JackEnable)

#Uncomment when Port Audio is integrated
#if(PortAudioOutput)
#    include_directories(${PORTAUDIO_INCLUDE_DIR})
#    list(APPEND zynaddsubfx_nio_SRCS PaEngine.cpp)
#    list(zynaddsubfx_nio_lib ${PORTAUDIO_LIBRARIES})
#endif(PortAudioOutput)

if(AlsaEnable)
    list(APPEND zynaddsubfx_nio_SRCS AlsaEngine.cpp)
    list(APPEND zynaddsubfx_nio_lib ${ASOUND_LIBRARY})
endif(AlsaEnable)

if(OssEnable)
    list(APPEND zynaddsubfx_nio_SRCS OssEngine.cpp)
endif(OssEnable)


add_library(zynaddsubfx_nio STATIC
    ${zynaddsubfx_nio_SRCS} 
    )

target_link_libraries(zynaddsubfx_nio)
