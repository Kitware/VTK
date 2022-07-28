cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
project(MinimalQtVTKApp)

find_package(VTK
  COMPONENTS
    CommonCore
    GUISupportQt)
if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

find_package(Qt5 COMPONENTS Core Widgets)
if (NOT TARGET Qt5::Core OR NOT TARGET Qt5::Widgets)
  message("Skipping example: ${Qt5_NOT_FOUND_MESSAGE}")
  return ()
endif ()

# Set your files and resources here
set(Srcs
  MinimalQtVTKApp.cxx)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)


# CMAKE_AUTOMOC in ON so the MocHdrs will be automatically wrapped.
add_executable(MinimalQtVTKApp
  ${Srcs})
target_link_libraries(MinimalQtVTKApp
  PRIVATE
    ${VTK_LIBRARIES}
    Qt5::Core
    Qt5::Widgets)
vtk_module_autoinit(
  TARGETS MinimalQtVTKApp
  MODULES ${VTK_LIBRARIES})
