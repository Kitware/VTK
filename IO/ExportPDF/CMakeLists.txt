include(vtkObjectFactory)
vtk_object_factory_configure("${vtk_module_overrides}")

set(Module_SRCS
  vtkPDFContextDevice2D.cxx
  vtkPDFExporter.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
)

vtk_module_library(${vtk-module} ${Module_SRCS})
