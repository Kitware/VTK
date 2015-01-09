include(vtkObjectFactory)

set(Module_SRCS
  vtkMatplotlibMathTextUtilities.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
  )
set_source_files_properties(${vtk-module}ObjectFactory WRAP_EXCLUDE)

# Override some rendering classes. Create object factory:
list(APPEND vtk_module_overrides "vtkMathTextUtilities")
set(vtk_module_vtkMathTextUtilities_override "vtkMatplotlibMathTextUtilities")
vtk_object_factory_configure("${vtk_module_overrides}")

vtk_module_library(${vtk-module} ${Module_SRCS})
