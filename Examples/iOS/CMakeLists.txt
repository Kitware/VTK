# copy the example to the binary tree where they can be built manually using xcode

if (TRUE)
  add_custom_target(iOS-examples-rm ALL
    COMMAND ${CMAKE_COMMAND} -E remove_directory
            "${CMAKE_CURRENT_BINARY_DIR}/GLPaint"
    COMMAND ${CMAKE_COMMAND} -E remove_directory
            "${CMAKE_CURRENT_BINARY_DIR}/Surfaces")

   add_custom_target(iOS-examples-copy ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/GLPaint"
            "${CMAKE_CURRENT_BINARY_DIR}/GLPaint"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Surfaces"
            "${CMAKE_CURRENT_BINARY_DIR}/Surfaces")

  add_dependencies(iOS-examples-copy iOS-examples-rm)
else ()
  add_subdirectory(MaceVTK/MaceVTK)
  add_custom_target(iOS-examples-copy ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/MaceVTK"
            "${CMAKE_CURRENT_BINARY_DIR}/MaceVTK")
endif ()
