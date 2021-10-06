cmake_minimum_required(VERSION 3.8)
project(UsingVTK)

include(GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

# We just need the CommonCore module here.
find_package(VTK COMPONENTS CommonCore CommonDataModel)
if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

include(CTest)

add_library(vtk_using_lib uses_vtk.cxx)
# VTK modules act just like CMake imported targets.
target_link_libraries(vtk_using_lib PRIVATE VTK::CommonCore VTK::CommonDataModel)
target_include_directories(vtk_using_lib PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
include(GenerateExportHeader)
generate_export_header(vtk_using_lib)

add_executable(vtk_using_exe uses_vtk_exe.cxx)
target_link_libraries(vtk_using_exe PRIVATE vtk_using_lib)

add_test(NAME vtk_using_exe
  COMMAND vtk_using_exe)
