set(Deprecated_DEPS)

if(NOT VTK_LEGACY_REMOVE)
  list(APPEND Deprecated_DEPS vtkViewsGeovis)
endif()

vtk_module(vtkGeovisCore
  GROUPS
    Rendering
  TEST_DEPENDS
    ${Deprecated_DEPS}
    vtkViewsInfovis
    vtkRenderingCore
    vtkRenderingOpenGL2
    vtkTestingRendering
    vtkInteractionStyle
  DEPENDS
    vtkCommonCore
    vtkCommonDataModel
    vtkCommonExecutionModel
    vtkCommonTransforms
    vtkInfovisCore
    vtkInteractionStyle
    vtkInteractionWidgets
    vtkRenderingCore
    vtkViewsCore
    vtklibproj
  PRIVATE_DEPENDS
    vtkCommonSystem
    vtkFiltersCore
    vtkFiltersGeneral
    vtkIOImage
    vtkIOXML
    vtkImagingCore
    vtkImagingSources
    vtkInfovisLayout
  )
