set(classes
  vtkOpenGLMoleculeMapper
  )

# Now for the object factory.
set(opengl_overrides
  MoleculeMapper
  )

foreach(_override ${opengl_overrides})
  vtk_object_factory_declare(
    BASE vtk${_override}
    OVERRIDE vtkOpenGL${_override})
endforeach()

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKDOMAINSCHEMISTRYOPENGL2_EXPORT")

vtk_module_add_module(VTK::DomainsChemistryOpenGL2
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header})
