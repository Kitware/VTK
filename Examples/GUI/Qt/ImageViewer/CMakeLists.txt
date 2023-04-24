cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
project(QtImageViewer)

find_package(VTK
  COMPONENTS
    GUISupportQt
    IOImage
    InteractionImage
    RenderingOpenGL2
    TestingCore)
if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

find_package("Qt${VTK_QT_VERSION}" COMPONENTS Widgets)
if (NOT TARGET "Qt${VTK_QT_VERSION}::Widgets")
  message("Skipping example: ${Qt${VTK_QT_VERSION}_NOT_FOUND_MESSAGE}")
  return ()
endif ()

set(Srcs main.cxx)

add_executable(qtimageviewer
  ${Srcs})
target_link_libraries(qtimageviewer
  PRIVATE
    ${VTK_LIBRARIES}
    "Qt${VTK_QT_VERSION}::Widgets")
vtk_module_autoinit(
  TARGETS qtimageviewer
  MODULES ${VTK_LIBRARIES})
