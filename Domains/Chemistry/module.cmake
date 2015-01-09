if(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
    set(extra_opengl_depend  vtkDomainsChemistry${VTK_RENDERING_BACKEND})
endif()

vtk_module(vtkDomainsChemistry
  GROUPS
    StandAlone
  DEPENDS
    vtkCommonDataModel
    vtkRenderingCore
  PRIVATE_DEPENDS
    vtkIOXML
    vtkFiltersSources
  TEST_DEPENDS
    vtkTestingCore
    vtkTestingRendering
    vtkInteractionStyle
    vtkRendering${VTK_RENDERING_BACKEND}
    ${extra_opengl_depend}
  )
