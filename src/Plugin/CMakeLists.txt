add_compile_options(-Wno-parentheses) # disable warnings from Lv2 headers

add_executable(lv2-ttl-generator ${CMAKE_SOURCE_DIR}/DPF/utils/lv2-ttl-generator/lv2_ttl_generator.c)

CHECK_LIBRARY_EXISTS(dl dlopen "" LIBDL_FOUND)

IF(LIBDL_FOUND)
target_link_libraries(lv2-ttl-generator "dl")
ELSE(LIBDL_FOUND)
# FreeBSD has dlopen as part of LibC
CHECK_LIBRARY_EXISTS(c dlopen "" LIBDL_FOUND)
ENDIF(LIBDL_FOUND)

CHECK_LIBRARY_EXISTS(pthread pthread_create "" LIBPTHREAD_FOUND)

IF(LIBPTHREAD_FOUND)
# Some X11 libGL libraries require -lpthread on FreeBSD
target_link_libraries(lv2-ttl-generator "pthread")
ENDIF(LIBPTHREAD_FOUND)

IF(LIBDL_FOUND)
add_subdirectory(AlienWah)
add_subdirectory(Chorus)
add_subdirectory(Distortion)
add_subdirectory(DynamicFilter)
add_subdirectory(Echo)
add_subdirectory(Phaser)
add_subdirectory(Reverb)
ENDIF(LIBDL_FOUND)

add_subdirectory(ZynAddSubFX)
