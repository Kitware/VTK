# The tests can be built without matplotlib, but we add some additional tests
# if it is enabled.
set(optional_test_depends)
if(${Module_vtkRenderingMatplotlib})
  set(optional_test_depends "vtkRenderingMatplotlib")
endif()

vtk_module(vtkRenderingFreeType
  IMPLEMENTS
    vtkRenderingCore
  GROUPS
    Rendering
  DEPENDS
    vtkRenderingCore
    vtkfreetype
  TEST_DEPENDS
    ${optional_test_depends}
    vtkTestingRendering
    vtkViewsContext2D
    vtkRendering${VTK_RENDERING_BACKEND}
    vtkRenderingContext${VTK_RENDERING_BACKEND}
  KIT
    vtkRendering
  )
