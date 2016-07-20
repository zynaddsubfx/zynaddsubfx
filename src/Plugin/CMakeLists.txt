
add_executable(lv2-ttl-generator ${CMAKE_SOURCE_DIR}/DPF/utils/lv2-ttl-generator/lv2_ttl_generator.c)

CHECK_LIBRARY_EXISTS(dl dlopen "" LIBDL_FOUND)

IF(LIBDL_FOUND)
target_link_libraries(lv2-ttl-generator "dl")
add_subdirectory(AlienWah)
add_subdirectory(Chorus)
add_subdirectory(Distortion)
add_subdirectory(DynamicFilter)
add_subdirectory(Echo)
add_subdirectory(Phaser)
add_subdirectory(Reverb)
ENDIF(LIBDL_FOUND)

add_subdirectory(ZynAddSubFX)
