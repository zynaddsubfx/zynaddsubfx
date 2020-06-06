function(cp_script script_name)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${script_name}
		${CMAKE_CURRENT_BINARY_DIR}/${script_name} COPYONLY)
endfunction()

function(quick_test test_name link)
    add_executable(${test_name} "${test_name}.cpp")
    add_test(NAME ${test_name}
             COMMAND ${test_name})
     target_link_libraries(${test_name} ${link} ${ARGN})
endfunction()

#for tests looking for files stored in the source dir
add_definitions(-DSOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

#Extra libraries added to make test and full compilation use the same library
#links for quirky compilers
set(test_lib zynaddsubfx_core ${GUI_LIBRARIES} ${ZLIB_LIBRARY} ${FFTW3F_LIBRARIES}
    ${MXML_LIBRARIES} pthread "-Wl,--no-as-needed -lpthread")

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
quick_test(TensorTest       ${test_lib})
quick_test(TriggerTest      ${test_lib})
quick_test(UnisonTest       ${test_lib})
quick_test(WatchTest        ${test_lib})
quick_test(WaveTableTest    ${test_lib})
quick_test(XMLwrapperTest   ${test_lib})

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
    target_link_libraries(ins-test ${test_lib} rt)

    if(LIBLO_FOUND)
        cp_script(check-ports.rb)
        add_test(PortChecker check-ports.rb)
    endif()
    add_executable(save-osc SaveOSC.cpp)
    target_link_libraries(save-osc
        zynaddsubfx_core zynaddsubfx_nio
        zynaddsubfx_gui_bridge
        ${GUI_LIBRARIES} ${NIO_LIBRARIES} ${AUDIO_LIBRARIES}
        ${PLATFORM_LIBRARIES})
    #this will be replaced with a for loop when the code will get more stable:
    add_test(SaveOsc save-osc ${CMAKE_CURRENT_SOURCE_DIR}/../../instruments/examples/Arpeggio\ 1.xmz)

endif()

