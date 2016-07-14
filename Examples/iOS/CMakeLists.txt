# copy the example to the binary tree where they can be built manually using xcode

add_custom_target(iOS-examples-rm ALL
  COMMAND ${CMAKE_COMMAND}
    -D BIN_DIR="${CMAKE_CURRENT_BINARY_DIR}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/CleanExamples.cmake"
    )

 add_custom_target(iOS-examples-copy ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          \"${CMAKE_CURRENT_SOURCE_DIR}/GLPaint\"
          \"${CMAKE_CURRENT_BINARY_DIR}/GLPaint\"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          \"${CMAKE_CURRENT_SOURCE_DIR}/Surfaces\"
          \"${CMAKE_CURRENT_BINARY_DIR}/Surfaces\"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          \"${CMAKE_CURRENT_SOURCE_DIR}/VolumeRender\"
          \"${CMAKE_CURRENT_BINARY_DIR}/VolumeRender\"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          \"${CMAKE_CURRENT_SOURCE_DIR}/PlaneView\"
          \"${CMAKE_CURRENT_BINARY_DIR}/PlaneView\")

add_dependencies(iOS-examples-copy iOS-examples-rm)
