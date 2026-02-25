function(cp_script script_name)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${script_name}
        ${CMAKE_CURRENT_BINARY_DIR}/${script_name} COPYONLY)
endfunction()

function(quick_test test_name link)
    add_executable(${test_name} "${test_name}.cpp")
    add_test(NAME ${test_name}
             COMMAND ${test_name})
    target_link_libraries(${test_name} ${link} ${ARGN})
    if(Werror)
        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
            set_target_properties(${test_name} PROPERTIES COMPILE_WARNINGS_AS_ERRORS ON)
	 else()
            if(NOT WIN32)  # For Windows, just allow warnings for now
                target_compile_options(${test_name} PUBLIC -Werror)
            endif()
        endif()
    endif()
endfunction()

#for tests looking for files stored in the source dir
add_definitions(-DSOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

#Extra libraries added to make test and full compilation use the same library
#links for quirky compilers
set(test_lib zynaddsubfx_core ${GUI_LIBRARIES} ${ZLIB_LIBRARY} ${FFTW3F_LIBRARIES}
    ${MXML_LIBRARIES} pthread )

message(STATUS "Linking tests with: ${test_lib}")

quick_test(AdNoteTest       ${test_lib})
quick_test(AllocatorTest    ${test_lib})
quick_test(ControllerTest   ${test_lib})
quick_test(EchoTest         ${test_lib})
quick_test(EffectTest       ${test_lib})
quick_test(KitTest          ${test_lib})
quick_test(MemoryStressTest ${test_lib})
quick_test(MicrotonalTest   ${test_lib})
quick_test(MsgParseTest     ${test_lib})
quick_test(OscilGenTest     ${test_lib})
quick_test(PadNoteTest      ${test_lib})
quick_test(PortamentoTest   ${test_lib})
quick_test(RandTest         ${test_lib})
quick_test(SubNoteTest      ${test_lib})
quick_test(TriggerTest      ${test_lib})
quick_test(UnisonTest       ${test_lib})
quick_test(WatchTest        ${test_lib})
quick_test(XMLwrapperTest   ${test_lib})
quick_test(ReverseTest   ${test_lib})

quick_test(PluginTest     zynaddsubfx_core zynaddsubfx_nio
                          zynaddsubfx_gui_bridge
                          ${GUI_LIBRARIES} ${NIO_LIBRARIES} ${AUDIO_LIBRARIES}
                          ${PLATFORM_LIBRARIES})
quick_test(MiddlewareTest zynaddsubfx_core zynaddsubfx_nio
                          zynaddsubfx_gui_bridge
                          ${GUI_LIBRARIES} ${NIO_LIBRARIES} ${AUDIO_LIBRARIES}
                          ${PLATFORM_LIBRARIES})

#Testbed app

if(NOT (${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))

    #std::thread issues with mingw vvvvv
    quick_test(MqTest           ${test_lib})
    #same std::thread mingw issue
    quick_test(MessageTest zynaddsubfx_core zynaddsubfx_nio
                           zynaddsubfx_gui_bridge
                           ${GUI_LIBRARIES} ${NIO_LIBRARIES} ${AUDIO_LIBRARIES}
                           ${PLATFORM_LIBRARIES})

    add_executable(ins-test InstrumentStats.cpp)
    target_link_libraries(ins-test ${test_lib})
    if(HAVE_LIBRT)
        target_link_libraries(ins-test rt)
    endif()

    if(LIBLO_FOUND)
        quick_test(PortChecker lo-server zynaddsubfx_core zynaddsubfx_nio
                   zynaddsubfx_gui_bridge
                   ${GUI_LIBRARIES} ${NIO_LIBRARIES} ${AUDIO_LIBRARIES}
                   ${PLATFORM_LIBRARIES})
        target_include_directories(PortChecker PRIVATE ${RTOSC_TEST_INCLUDE_DIR})
        target_link_directories(PortChecker PRIVATE ${RTOSC_TEST_LIB_DIR})
    endif()

    add_executable(save-osc SaveOSC.cpp)
    if(LIBGIT2_FOUND)
        target_include_directories(save-osc PRIVATE ${LIBGIT2_INCLUDE_DIRS})
        target_link_libraries(save-osc
            zynaddsubfx_core zynaddsubfx_nio
            zynaddsubfx_gui_bridge
            ${GUI_LIBRARIES} ${NIO_LIBRARIES} ${AUDIO_LIBRARIES}
            ${LIBGIT2_LIBRARIES}
            ${PLATFORM_LIBRARIES})
    else()
        target_compile_definitions(save-osc PRIVATE -DZYN_GIT_WORKTREE="${CMAKE_SOURCE_DIR}")
        target_link_libraries(save-osc
            zynaddsubfx_core zynaddsubfx_nio
            zynaddsubfx_gui_bridge
            ${GUI_LIBRARIES} ${NIO_LIBRARIES} ${AUDIO_LIBRARIES}
            ${PLATFORM_LIBRARIES})
    endif()
    # test all presets
    add_test(SaveOscPresets save-osc)
    # test the most insane XIZ file
    add_test(SaveOscBigFile save-osc ${CMAKE_CURRENT_SOURCE_DIR}/../../instruments/banks/olivers-100/0032-Drum\ Kit.xiz)
    # not realized, because it takes 15 minutes each time:
    if (CompileExtensiveTests)
        add_test(SaveOscAllFiles save-osc test-all)
    endif()

endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    execute_process(
        COMMAND git rev-parse --is-inside-work-tree
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE IS_GIT_REPO
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(IS_GIT_REPO STREQUAL "true")
        add_test(TrailingWhitespace ${CMAKE_SOURCE_DIR}/src/Tests/trailing-whitespace)
    endif()
endif()

