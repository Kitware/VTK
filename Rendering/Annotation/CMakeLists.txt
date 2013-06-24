set(Module_SRCS
  vtkAnnotatedCubeActor.cxx
  vtkArcPlotter.cxx
  vtkAxesActor.cxx
  vtkAxisActor2D.cxx
  vtkAxisActor.cxx
  vtkAxisFollower.cxx
  vtkBarChartActor.cxx
  vtkCaptionActor2D.cxx
  vtkCornerAnnotation.cxx
  vtkCubeAxesActor2D.cxx
  vtkCubeAxesActor.cxx
  vtkLeaderActor2D.cxx
  vtkLegendBoxActor.cxx
  vtkLegendScaleActor.cxx
  vtkPieChartActor.cxx
  vtkPolarAxesActor.cxx
  vtkProp3DAxisFollower.cxx
  vtkScalarBarActor.cxx
  vtkSpiderPlotActor.cxx
  vtkXYPlotActor.cxx
  )

set(${vtk-module}_HDRS
  vtkScalarBarActorInternal.h
  )

set_source_files_properties(
  vtkScalarBarActorInternal.h
  HEADER_FILE_ONLY
  WRAP_EXCLUDE
  )

vtk_module_library(vtkRenderingAnnotation ${Module_SRCS})
