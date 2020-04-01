cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(Charts)

find_package(VTK
  COMPONENTS
    ChartsCore
  OPTIONAL_COMPONENTS
    CommonSystem
    GUISupportQt
    InfovisCore
    RenderingOpenGL2
    ViewsInfovis
    ViewsContext2D
    ViewsQt)
if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

if (TARGET VTK::InfovisCore AND TARGET VTK::ViewsInfovis AND TARGET VTK::ViewsContext2D)
  add_executable(GraphItem
    GraphItem.cxx
    vtkGraphItem.cxx
    vtkGraphItem.h)
  target_link_libraries(GraphItem
    PRIVATE
      VTK::ChartsCore
      VTK::InfovisCore
      VTK::ViewsInfovis
      VTK::ViewsContext2D)
  vtk_module_autoinit(
    TARGETS GraphItem
    MODULES VTK::ChartsCore
            VTK::InfovisCore
            VTK::ViewsInfovis
            VTK::ViewsContext2D)
endif ()

if (TARGET VTK::ViewsContext2D)
  add_executable(PiecewiseItem
    PiecewiseItem.cxx)
  target_link_libraries(PiecewiseItem
    PRIVATE
      VTK::ChartsCore
      VTK::ViewsContext2D)
  vtk_module_autoinit(
    TARGETS PiecewiseItem
    MODULES VTK::ChartsCore
            VTK::ViewsContext2D)
endif ()

if (TARGET VTK::CommonSystem AND TARGET VTK::GUISupportQt AND TARGET VTK::ViewsContext2D AND TARGET VTK::ViewsQt)
  add_executable(QChartTable
    QChartTable.cxx)
  target_link_libraries(QChartTable
    PRIVATE
      VTK::ChartsCore
      VTK::CommonSystem
      VTK::GUISupportQt
      VTK::ViewsContext2D
      VTK::ViewsQt)

  add_executable(QScalarsToColors
    QScalarsToColors.cxx)
  target_link_libraries(QScalarsToColors
    PRIVATE
      VTK::ChartsCore
      VTK::CommonSystem
      VTK::GUISupportQt
      VTK::ViewsContext2D
      VTK::ViewsQt)

  vtk_module_autoinit(
    TARGETS QChartTable
            QScalarsToColors
    MODULES VTK::ChartsCore
            VTK::CommonSystem
            VTK::GUISupportQt
            VTK::ViewsContext2D
            VTK::ViewsQt)
endif ()
