cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
project(QtVTKRenderWindows)

find_package(VTK
  COMPONENTS
    CommonCore
    GUISupportQt
    IOImage
    ImagingColor
    ImagingGeneral
    InteractionImage
    InteractionStyle
    InteractionWidgets)
if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

find_package(Qt5 COMPONENTS Widgets)
if (NOT TARGET Qt5::Widgets)
  message("Skipping example: ${Qt5_NOT_FOUND_MESSAGE}")
  return ()
endif ()

# Set your files and resources here
set(Srcs
  QtVTKRenderWindowsApp.cxx
  QtVTKRenderWindows.cxx)

set(Hdrs
  QtVTKRenderWindows.h)

set(UIs
  QtVTKRenderWindows.ui)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# CMAKE_AUTOMOC in ON so the MocHdrs will be automatically wrapped.
add_executable(QtVTKRenderWindows
  ${Srcs} ${Hdrs} ${UIs})
target_link_libraries(QtVTKRenderWindows
  PRIVATE
    ${VTK_LIBRARIES}
    Qt5::Widgets)
vtk_module_autoinit(
  TARGETS QtVTKRenderWindows
  MODULES ${VTK_LIBRARIES})
