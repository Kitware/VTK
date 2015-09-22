set(Module_SRCS
  vtkAnnotatedCubeActor.cxx
  vtkArcPlotter.cxx
  vtkAxesActor.cxx
  vtkAxisActor2D.cxx
  vtkAxisActor.cxx
  vtkAxisFollower.cxx
  vtkBarChartActor.cxx
  vtkCaptionActor2D.cxx
  vtkConvexHull2D.cxx
  vtkCornerAnnotation.cxx
  vtkCubeAxesActor2D.cxx
  vtkCubeAxesActor.cxx
  vtkGraphAnnotationLayersFilter.cxx
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
  PROPERTIES
    HEADER_FILE_ONLY 1
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
  )

vtk_module_library(vtkRenderingAnnotation ${Module_SRCS})
