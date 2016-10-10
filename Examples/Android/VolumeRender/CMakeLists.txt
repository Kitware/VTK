cmake_minimum_required(VERSION 3.7)
project(VolumeRender)

if (CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "VTK Android does not support in-source builds :) .")
endif ()

find_package(VTK COMPONENTS
  vtkInteractionStyle
  vtkRenderingOpenGL2
  vtkRenderingVolumeOpenGL2
  vtkRenderingFreeType
)
include(${VTK_USE_FILE})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libs/${CMAKE_ANDROID_ARCH_ABI})
message(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
add_subdirectory(jni)

# find android
find_program(ANDROID_EXECUTABLE
  NAMES android
  DOC   "The android command-line tool")
if(NOT ANDROID_EXECUTABLE)
  message(FATAL_ERROR "Can not find android command line tool: android")
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/AndroidManifest.xml"
  "${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml"
  COPYONLY)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/ant.properties.in"
  "${CMAKE_CURRENT_BINARY_DIR}/ant.properties"
  @ONLY)

add_custom_target(VolumeRender-ant-configure ALL
  COMMAND "${ANDROID_EXECUTABLE}"
          update project
          --name VolumeRender
          --path "${CMAKE_CURRENT_SOURCE_DIR}"
          --target "android-${CMAKE_SYSTEM_VERSION}"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/build.xml"
          "${CMAKE_CURRENT_BINARY_DIR}/build.xml"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/local.properties"
          "${CMAKE_CURRENT_BINARY_DIR}/local.properties"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/project.properties"
          "${CMAKE_CURRENT_BINARY_DIR}/project.properties"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/proguard-project.txt"
          "${CMAKE_CURRENT_BINARY_DIR}/proguard-project.txt"
  COMMAND "${CMAKE_COMMAND}" -E remove
          "${CMAKE_CURRENT_SOURCE_DIR}/build.xml"
          "${CMAKE_CURRENT_SOURCE_DIR}/local.properties"
          "${CMAKE_CURRENT_SOURCE_DIR}/project.properties"
          "${CMAKE_CURRENT_SOURCE_DIR}/proguard-project.txt"
  WORKING_DIRECTORY
          "${CMAKE_CURRENT_BINARY_DIR}")

add_dependencies(VolumeRender-ant-configure VolumeRender)

#find ant
find_program(ANT_EXECUTABLE
  NAMES ant
  DOC   "The ant build tool")
if(NOT ANT_EXECUTABLE)
  message(FATAL_ERROR "Can not find ant build tool: ant")
endif()

add_custom_target(VolumeRender-apk-release ALL
  COMMAND ${ANT_EXECUTABLE}
          -file "${CMAKE_CURRENT_BINARY_DIR}/build.xml"
          release)
add_dependencies(VolumeRender-apk-release
  VolumeRender-ant-configure
  VolumeRender)

add_custom_target(VolumeRender-apk-debug ALL
  COMMAND ${ANT_EXECUTABLE}
          -file "${CMAKE_CURRENT_BINARY_DIR}/build.xml"
          debug)
add_dependencies(VolumeRender-apk-debug
  VolumeRender-apk-release
  VolumeRender-ant-configure
  VolumeRender)
