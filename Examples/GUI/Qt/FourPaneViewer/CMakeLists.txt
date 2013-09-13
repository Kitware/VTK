cmake_minimum_required(VERSION 2.6)

project(QtVTKRenderWindows)

if(NOT VTK_BINARY_DIR)
  find_package(VTK)
  if(NOT VTK_DIR)
    message(FATAL_ERROR "Please set VTK_DIR.")
  endif()
  include(${VTK_USE_FILE})
endif()

# Use what VTK built with
set(QT_QMAKE_EXECUTABLE ${VTK_QT_QMAKE_EXECUTABLE} CACHE FILEPATH "")
set(QT_MOC_EXECUTABLE ${VTK_QT_MOC_EXECUTABLE} CACHE FILEPATH "")
set(QT_UIC_EXECUTABLE ${VTK_QT_UIC_EXECUTABLE} CACHE FILEPATH "")
find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})

# Set your files and resources here
set(QtVTKRenderWindowsSrcs QtVTKRenderWindowsApp.cxx QtVTKRenderWindows.cxx)
set(QtVTKRenderWindowsUI QtVTKRenderWindows.ui)
set(QtVTKRenderWindowsHeaders QtVTKRenderWindows.h)

include_directories(
  ${QT_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

qt4_wrap_ui(UISrcs ${QtVTKRenderWindowsUI})
qt4_wrap_cpp(MOCSrcs ${QtVTKRenderWindowsHeaders} )

source_group("Resources" FILES
  ${QtVTKRenderWindowsUI}
)

source_group("Generated" FILES
  ${UISrcs}
  ${MOCSrcs}
  ${RCS_SOURCES}
)

add_executable(QtVTKRenderWindows ${QtVTKRenderWindowsSrcs} ${UISrcs} ${MOCSrcs})
target_link_libraries(QtVTKRenderWindows ${QT_LIBRARIES} ${VTK_LIBRARIES})
