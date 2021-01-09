set(classes
  vtkQImageToImageSource
  vtkQtInitialization
  vtkQtLabelRenderStrategy
  vtkQtStringToImage
  vtkQtTreeRingLabelMapper
  )

vtk_module_add_module(VTK::RenderingQt
  CLASSES ${classes})

include(vtkQt)
vtk_module_find_package(
  PACKAGE     Qt${VTK_QT_MAJOR_VERSION}
  COMPONENTS  Widgets
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         Qt${VTK_QT_MAJOR_VERSION}_VERSION)
vtk_module_link(VTK::RenderingQt PUBLIC Qt${VTK_QT_MAJOR_VERSION}::Widgets)
