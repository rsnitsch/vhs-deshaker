add_executable(vhs-deshaker
               main.cpp
               correct_frame.cpp
               process_single_threaded.cpp
               StdoutVideoWriter.cpp)

target_link_libraries(vhs-deshaker ${OpenCV_LIBS})

install(TARGETS vhs-deshaker)

if(WIN32)
    get_target_property(opencv_world_dll_source_filepath opencv_world IMPORTED_LOCATION_RELEASE)
    install(FILES ${opencv_world_dll_source_filepath} DESTINATION bin)

    get_target_property(opencv_world_dll_source_filepath opencv_world IMPORTED_LOCATION_DEBUG)
    install(FILES ${opencv_world_dll_source_filepath} DESTINATION bin)

    file(GLOB opencv_ffmpeg_dll "${OpenCV_DIR}/x64/vc16/bin/opencv_videoio_ffmpeg*.dll")
    install(FILES ${opencv_ffmpeg_dll} DESTINATION bin)
endif()
