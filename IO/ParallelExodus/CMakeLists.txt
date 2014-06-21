include(vtkObjectFactory)

set(Module_SRCS
  vtkPExodusIIReader.cxx
  vtkPExodusIIWriter.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
  )
set_source_files_properties(${vtk-module}ObjectFactory WRAP_EXCLUDE)

# Now to generate our object factory.
vtk_add_override(vtkExodusIIWriter vtkPExodusIIWriter)
vtk_object_factory_configure("${vtk_module_overrides}")

vtk_module_library(vtkIOParallelExodus ${Module_SRCS})
