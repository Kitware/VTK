if(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
    set(extra_opengl_depend  vtkDomainsChemistry${VTK_RENDERING_BACKEND})
endif()
vtk_module(vtkDomainsChemistry
  GROUPS
    StandAlone
  TEST_DEPENDS
    vtkIOGeometry
    vtkIOLegacy
    vtkTestingCore
    vtkTestingRendering
    vtkInteractionStyle
    vtkRendering${VTK_RENDERING_BACKEND}
    ${extra_opengl_depend}
  DEPENDS
    vtkCommonCore
    vtkCommonDataModel
    vtkCommonExecutionModel
    vtkIOLegacy
    vtkIOXMLParser
    vtkRenderingCore
  PRIVATE_DEPENDS
    vtkCommonTransforms
    vtkFiltersCore
    vtkFiltersGeneral
    vtkFiltersSources
    vtksys
  )
