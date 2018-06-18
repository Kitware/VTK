cmake_minimum_required(VERSION 3.3...3.12 FATAL_ERROR)

project(QtImageViewer)

find_package(VTK COMPONENTS
  vtkGUISupportQt
  vtkIOImage
  vtkInteractionImage
  vtkRenderingOpenGL2
  vtkTestingCore
)
include(${VTK_USE_FILE})

if("${VTK_QT_VERSION}" STREQUAL "")
  message(FATAL_ERROR "VTK was not built with Qt")
endif()

set( Srcs main.cxx )

find_package(Qt5 COMPONENTS Core REQUIRED QUIET)

add_executable(qtimageviewer ${Srcs})
qt5_use_modules(qtimageviewer Core Gui Widgets)
target_link_libraries(qtimageviewer ${VTK_LIBRARIES})
