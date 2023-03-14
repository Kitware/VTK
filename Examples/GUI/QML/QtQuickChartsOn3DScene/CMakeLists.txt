cmake_minimum_required(VERSION 3.14)

set(MYNAME QtQuickChartsOn3DScene)

project(${MYNAME} VERSION 1.0)

set(PROJECT_SOURCES
  main.cpp
  main.qml
  qml.qrc
)

find_package(VTK
  COMPONENTS
    CommonCore
    CommonColor
    ChartsCore
    GUISupportQtQuick
    RenderingContextOpenGL2)

if (NOT VTK_FOUND)
  message("Skipping ${MYNAME}: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif()

include(../Common.cmake)

target_link_libraries(${MYNAME} PRIVATE ${VTK_LIBRARIES})
vtk_module_autoinit( TARGETS ${MYNAME} MODULES ${VTK_LIBRARIES} )
